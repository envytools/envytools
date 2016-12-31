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
		if (!nv04_pgraph_is_nv25p(&chipset))
			insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
		else
			egrobj[0] = exp.ctx_switch[0];
		insrt(egrobj[0], 15, 3, val);
		exp.ctx_cache[subc][0] = exp.ctx_switch[0];
		insrt(exp.ctx_cache[subc][0], 15, 3, val);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[0] = exp.ctx_cache[subc][0];
	}
}

void MthdDither::emulate_mthd() {
	uint32_t rval = 0;
	if ((val & 3) == 0)
		rval = 1;
	if ((val & 3) == 1)
		rval = 2;
	if ((val & 3) == 2)
		rval = 3;
	if ((val & 3) == 3)
		rval = 3;
	if (!extr(exp.nsource, 1, 1)) {
		if (!nv04_pgraph_is_nv25p(&chipset))
			insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
		else
			egrobj[0] = exp.ctx_switch[0];
		insrt(egrobj[0], 20, 2, rval);
		exp.ctx_cache[subc][0] = exp.ctx_switch[0];
		insrt(exp.ctx_cache[subc][0], 20, 2, rval);
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
			if (!nv04_pgraph_is_nv25p(&chipset))
				insrt(egrobj[0], 24, 8, extr(exp.ctx_switch[0], 24, 8));
			else
				egrobj[0] = exp.ctx_switch[0];
			insrt(egrobj[0], 24, 1, 0);
		}
	} else {
		exp.intr |= 0x10;
		exp.fifo_enable = 0;
	}
}

void MthdDmaNotify::emulate_mthd() {
	uint32_t rval = val & 0xffff;
	int dcls = extr(pobj[0], 0, 12);
	if (dcls == 0x30)
		rval = 0;
	bool bad = false;
	if (dcls != 0x30 && dcls != 0x3d && dcls != 3)
		bad = true;
	if (extr(exp.notify, 24, 1))
		nv04_pgraph_blowup(&exp, 0x2000);
	if (bad && extr(exp.debug[3], 23, 1) && chipset.chipset != 5)
		nv04_pgraph_blowup(&exp, 2);
	if (!extr(exp.nsource, 1, 1)) {
		insrt(egrobj[1], 16, 16, rval);
	}
	if (bad && extr(exp.debug[3], 23, 1))
		nv04_pgraph_blowup(&exp, 2);
	if (!exp.nsource) {
		exp.ctx_cache[subc][1] = exp.ctx_switch[1];
		insrt(exp.ctx_cache[subc][1], 16, 16, rval);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[1] = exp.ctx_cache[subc][1];
	}
}

void MthdDmaGrobj::emulate_mthd() {
	uint32_t rval = val & 0xffff;
	int dcls = extr(pobj[0], 0, 12);
	if (dcls == 0x30)
		rval = 0;
	bool bad = false;
	if (dcls != 0x30 && dcls != 0x3d && dcls != ecls)
		bad = true;
	if (bad && extr(exp.debug[3], 23, 1) && chipset.chipset != 5)
		nv04_pgraph_blowup(&exp, 2);
	if (!extr(exp.nsource, 1, 1)) {
		insrt(egrobj[2], 16 * which, 16, rval);
		if (clr)
			insrt(egrobj[2], 16, 16, 0);
	}
	if (bad && extr(exp.debug[3], 23, 1))
		nv04_pgraph_blowup(&exp, 2);
	if (!exp.nsource) {
		exp.ctx_cache[subc][2] = exp.ctx_switch[2];
		insrt(exp.ctx_cache[subc][2], 16 * which, 16, rval);
		if (clr)
			insrt(exp.ctx_cache[subc][2], 16, 16, 0);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[2] = exp.ctx_cache[subc][2];
	}
	bool prot_err = false;
	bool check_prot = true;
	if (chipset.card_type >= 0x10)
		check_prot = dcls != 0x30;
	else if (chipset.chipset >= 5)
		check_prot = dcls == 2 || dcls == 0x3d;
	if (align && check_prot) {
		if (extr(pobj[1], 0, 8) != 0xff)
			prot_err = true;
		if (cls != 0x48 && extr(pobj[0], 20, 8))
			prot_err = true;
	}
	if (prot_err)
		nv04_pgraph_blowup(&exp, 4);
	if (check_prev && !extr(exp.ctx_switch[2], 0, 16))
		pgraph_state_error(&exp);
}

static void nv04_pgraph_set_ctx(struct pgraph_state *state, uint32_t grobj[4], uint32_t pobj[4], int ecls, int bit) {
	int ccls = extr(pobj[0], 0, 12);
	bool bad = ccls != ecls && ccls != 0x30;
	if (state->chipset.card_type < 0x10 && !extr(state->nsource, 1, 1)) {
		insrt(grobj[0], 8, 24, extr(state->ctx_switch[0], 8, 24));
		insrt(grobj[0], bit, 1, ccls != 0x30);
	}
	if (bad && extr(state->debug[3], 23, 1))
		nv04_pgraph_blowup(state, 2);
	if (!extr(state->nsource, 1, 1)) {
		if (state->chipset.card_type >= 0x10) {
			if (!nv04_pgraph_is_nv25p(&state->chipset))
				insrt(grobj[0], 8, 24, extr(state->ctx_switch[0], 8, 24));
			else
				grobj[0] = state->ctx_switch[0];
			insrt(grobj[0], bit, 1, ccls != 0x30);
		}
		int subc = extr(state->ctx_user, 13, 3);
		state->ctx_cache[subc][0] = state->ctx_switch[0];
		insrt(state->ctx_cache[subc][0], bit, 1, ccls != 0x30);
		if (extr(state->debug[1], 20, 1))
			state->ctx_switch[0] = state->ctx_cache[subc][0];
	}
}

void MthdCtxChroma::emulate_mthd() {
	int ecls = (is_new || chipset.card_type < 0x10 ? 0x57 : 0x17);
	nv04_pgraph_set_ctx(&exp, egrobj, pobj, ecls, 12);
}

void MthdCtxClip::emulate_mthd() {
	nv04_pgraph_set_ctx(&exp, egrobj, pobj, 0x19, 13);
}

void MthdCtxPattern::emulate_mthd() {
	int ecls = (is_new ? 0x44 : 0x18);
	nv04_pgraph_set_ctx(&exp, egrobj, pobj, ecls, 27);
}

void MthdCtxRop::emulate_mthd() {
	nv04_pgraph_set_ctx(&exp, egrobj, pobj, 0x43, 28);
}

void MthdCtxBeta::emulate_mthd() {
	nv04_pgraph_set_ctx(&exp, egrobj, pobj, 0x12, 29);
}

void MthdCtxBeta4::emulate_mthd() {
	nv04_pgraph_set_ctx(&exp, egrobj, pobj, 0x72, 30);
}

void MthdCtxSurf::emulate_mthd() {
	int ccls = extr(pobj[0], 0, 12);
	bool bad = false;
	int ecls = 0x58 + which;
	bad = ccls != ecls && ccls != 0x30;
	bool isswz = ccls == 0x52;
	if (nv04_pgraph_is_nv15p(&chipset) && ccls == 0x9e)
		isswz = true;
	if (chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
		insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
		insrt(egrobj[0], 25 + (which & 1), 1, ccls != 0x30);
		if (which == 0 || which == 2)
			insrt(egrobj[0], 14, 1, isswz);
	}
	if (bad && extr(exp.debug[3], 23, 1))
		nv04_pgraph_blowup(&exp, 2);
	if (!extr(exp.nsource, 1, 1)) {
		if (chipset.card_type >= 0x10) {
			if (!nv04_pgraph_is_nv25p(&chipset))
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
			else
				egrobj[0] = exp.ctx_switch[0];
			insrt(egrobj[0], 25 + (which & 1), 1, ccls != 0x30);
			if (which == 0 || which == 2)
				insrt(egrobj[0], 14, 1, isswz);
		}
		exp.ctx_cache[subc][0] = exp.ctx_switch[0];
		insrt(exp.ctx_cache[subc][0], 25 + (which & 1), 1, ccls != 0x30);
		if (which == 0 || which == 2)
			insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[0] = exp.ctx_cache[subc][0];
	}
}

void MthdCtxSurf2D::emulate_mthd() {
	int ccls = extr(pobj[0], 0, 12);
	bool bad = false;
	bad = ccls != 0x42 && ccls != 0x52 && ccls != 0x30;
	if (ccls == (extr(exp.debug[3], 16, 1) ? 0x82 : 0x62) && chipset.card_type >= 0x10 && new_ok)
		bad = false;
	if (nv04_pgraph_is_nv15p(&chipset) && ccls == 0x9e && new_ok)
		bad = false;
	bool isswz = ccls == 0x52;
	if (nv04_pgraph_is_nv15p(&chipset) && ccls == 0x9e)
		isswz = true;
	if (isswz && !swz_ok)
		bad = true;
	if (chipset.chipset >= 5 && chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
		insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
		insrt(egrobj[0], 25, 1, ccls != 0x30);
		insrt(egrobj[0], 14, 1, isswz);
	}
	if (bad && extr(exp.debug[3], 23, 1))
		nv04_pgraph_blowup(&exp, 2);
	if (!extr(exp.nsource, 1, 1)) {
		if (chipset.card_type >= 0x10) {
			if (!nv04_pgraph_is_nv25p(&chipset))
				insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
			else
				egrobj[0] = exp.ctx_switch[0];
			insrt(egrobj[0], 25, 1, ccls != 0x30);
			insrt(egrobj[0], 14, 1, isswz);
		}
		exp.ctx_cache[subc][0] = exp.ctx_switch[0];
		insrt(exp.ctx_cache[subc][0], 25, 1, ccls != 0x30);
		insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[0] = exp.ctx_cache[subc][0];
	}
}

void MthdCtxSurf3D::emulate_mthd() {
	int ccls = extr(pobj[0], 0, 12);
	bool bad = false;
	bad = ccls != 0x53 && ccls != 0x30;
	if (ccls == 0x93 && chipset.card_type >= 0x10 && new_ok)
		bad = false;
	bool isswz = ccls == 0x52;
	if (nv04_pgraph_is_nv15p(&chipset) && ccls == 0x9e)
		isswz = true;
	if (chipset.chipset >= 5 && chipset.card_type < 0x10 && !extr(exp.nsource, 1, 1)) {
		insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
		insrt(egrobj[0], 25, 1, ccls != 0x30);
		insrt(egrobj[0], 14, 1, isswz);
	}
	if (bad && extr(exp.debug[3], 23, 1))
		nv04_pgraph_blowup(&exp, 2);
	if (chipset.chipset < 5) {
		int subc = extr(exp.ctx_user, 13, 3);
		exp.ctx_cache[subc][0] = exp.ctx_switch[0];
		insrt(exp.ctx_cache[subc][0], 25, 1, ccls == 0x53);
		if (extr(exp.debug[1], 20, 1))
			exp.ctx_switch[0] = exp.ctx_cache[subc][0];
		if (!extr(exp.nsource, 1, 1))
			insrt(egrobj[0], 24, 8, extr(exp.ctx_cache[subc][0], 24, 8));
	} else {
		if (!extr(exp.nsource, 1, 1)) {
			if (chipset.card_type >= 0x10) {
				if (!nv04_pgraph_is_nv25p(&chipset))
					insrt(egrobj[0], 8, 24, extr(exp.ctx_switch[0], 8, 24));
				else
					egrobj[0] = exp.ctx_switch[0];
				insrt(egrobj[0], 25, 1, ccls != 0x30);
				insrt(egrobj[0], 14, 1, isswz);
			}
			int subc = extr(exp.ctx_user, 13, 3);
			exp.ctx_cache[subc][0] = exp.ctx_switch[0];
			insrt(exp.ctx_cache[subc][0], 25, 1, ccls != 0x30);
			insrt(exp.ctx_cache[subc][0], 14, 1, isswz);
			if (extr(exp.debug[1], 20, 1))
				exp.ctx_switch[0] = exp.ctx_cache[subc][0];
		}
	}
}

}
}
