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

static void adjust_orig_bundle(struct pgraph_state *state) {
	state->surf_unk800 = 0;
}

class MthdEmuCelsiusClip : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x8000);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (which == 0)
				exp.kelvin_bundle_clip_h = val;
			else
				exp.kelvin_bundle_clip_v = val;
			pgraph_kelvin_bundle(&exp, 0x6d + which, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdEmuCelsiusClip(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdKelvinClip : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (which == 0)
				exp.kelvin_bundle_clip_h = val;
			else
				exp.kelvin_bundle_clip_v = val;
			pgraph_kelvin_bundle(&exp, 0x6d + which, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
public:
	MthdKelvinClip(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdEmuCelsiusTexOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val &= ~0x7f;
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x7f);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_tex_offset[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x89 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 3, 2, 1);
			if (rnd() & 3)
				insrt(val, 5, 2, 1);
			if (!(rnd() & 3))
				insrt(val, 7, 5, 0x19 + rnd() % 4);
			if (rnd() & 1)
				insrt(val, 20, 4, extr(val, 16, 4));
			if (rnd() & 3)
				insrt(val, 24, 3, 3);
			if (rnd() & 3)
				insrt(val, 28, 3, 3);
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 12, 4, 1);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mips = extr(val, 12, 4);
		int su = extr(val, 16, 4);
		int sv = extr(val, 20, 4);
		int fmt = extr(val, 7, 5);
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (extr(val, 2, 1)) {
			if (su != sv)
				return false;
			if (su >= 0xa && (fmt == 6 || fmt == 7 || fmt == 0xb || fmt == 0xe || fmt == 0xf))
				return false;
			if (su >= 0xb)
				return false;
		}
		if (!extr(val, 3, 2) || extr(val, 3, 2) == 3)
			return false;
		if (!extr(val, 5, 2) || extr(val, 5, 2) == 3)
			return false;
		if (fmt == 0xd)
			return false;
		if (fmt >= 0x1d)
			return false;
		if (mips > 0xc || mips == 0)
			return false;
		if (fmt >= 0x19) {
			if (cls != 0x99)
				return false;
		}
		if (fmt >= 0x10) {
			if (extr(val, 2, 1))
				return false;
			if (extr(val, 24, 3) != 3)
				return false;
			if (extr(val, 28, 3) != 3)
				return false;
			if (mips != 1)
				return false;
		}
		if (su > 0xb || sv > 0xb)
			return false;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 3)
			return false;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 3)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			int mode = extr(val, 2, 1) ? 3 : 1;
			if (idx == 0)
				insrt(exp.kelvin_bundle_unk067, 0, 3, mode);
			else
				insrt(exp.kelvin_bundle_unk067, 5, 5, mode);
			pgraph_kelvin_bundle(&exp, 0x67, exp.kelvin_bundle_unk067, true);
			insrt(exp.kelvin_bundle_tex_wrap[idx], 0, 3, extr(val, 24, 3));
			insrt(exp.kelvin_bundle_tex_wrap[idx], 4, 1, extr(val, 27, 1));
			insrt(exp.kelvin_bundle_tex_wrap[idx], 8, 3, extr(val, 28, 3));
			insrt(exp.kelvin_bundle_tex_wrap[idx], 12, 1, extr(val, 31, 1));
			pgraph_kelvin_bundle(&exp, 0x6f + idx, exp.kelvin_bundle_tex_wrap[idx], true);
			int mips = extr(val, 12, 4);
			int su = extr(val, 16, 4);
			int sv = extr(val, 20, 4);
			if (mips > su && mips > sv)
				mips = std::max(su, sv) + 1;
			insrt(exp.kelvin_bundle_tex_format[idx], 1, 1, extr(val, 1, 1));
			insrt(exp.kelvin_bundle_tex_format[idx], 2, 1, extr(val, 2, 1));
			insrt(exp.kelvin_bundle_tex_format[idx], 4, 1, extr(val, 4, 1));
			insrt(exp.kelvin_bundle_tex_format[idx], 5, 1, extr(val, 6, 1));
			insrt(exp.kelvin_bundle_tex_format[idx], 8, 5, extr(val, 7, 5));
			insrt(exp.kelvin_bundle_tex_format[idx], 13, 2, 0);
			insrt(exp.kelvin_bundle_tex_format[idx], 16, 4, mips);
			insrt(exp.kelvin_bundle_tex_format[idx], 20, 4, su);
			insrt(exp.kelvin_bundle_tex_format[idx], 24, 4, sv);
			pgraph_kelvin_bundle(&exp, 0x81 + idx, exp.kelvin_bundle_tex_format[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexControl : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 31, 1))
			return false;
		if (extr(val, 5, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.kelvin_xf_mode_c[1], idx * 16, 1, extr(val, 30, 1));
			pgraph_kelvin_xf_mode(&exp);
			exp.kelvin_bundle_tex_control[idx] = val & 0x7fffffff;

			pgraph_kelvin_bundle(&exp, 0x73 + idx, exp.kelvin_bundle_tex_control[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexPitch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xffff;
			if (!(rnd() & 3)) {
				val &= 0xe00f0000;
			}
			if (!(rnd() & 3))
				val = 0;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0xffff) && !!(val & 0xfff80000);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_tex_pitch[idx] = val & 0xffff0000;
			pgraph_kelvin_bundle(&exp, 0x77 + idx, exp.kelvin_bundle_tex_pitch[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexUnk238 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_tex_unk238[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x7b + idx, exp.kelvin_bundle_tex_unk238[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= ~0xf000f800;
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 16, 1))
			return false;
		if (!extr(val, 0, 16) || extr(val, 0, 16) >= 0x800)
			return false;
		if (!extr(val, 17, 15) || extr(val, 17, 15) >= 0x800)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_tex_rect[idx] = val & 0x1fff1fff;
			pgraph_kelvin_bundle(&exp, 0x85 + idx, exp.kelvin_bundle_tex_rect[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 11, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 3)
				insrt(val, 28, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 13, 11))
			return false;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 6)
			return false;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.kelvin_bundle_tex_filter[idx], 0, 13, extr(val, 0, 13));
			insrt(exp.kelvin_bundle_tex_filter[idx], 16, 4, extr(val, 24, 4));
			insrt(exp.kelvin_bundle_tex_filter[idx], 20, 2, 0);
			insrt(exp.kelvin_bundle_tex_filter[idx], 24, 2, extr(val, 28, 2));
			insrt(exp.kelvin_bundle_tex_filter[idx], 26, 2, 0);
			pgraph_kelvin_bundle(&exp, 0x7d + idx, exp.kelvin_bundle_tex_filter[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexPalette : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x3c;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x3e);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_tex_palette[idx] = val & ~0x32;
			pgraph_kelvin_bundle(&exp, 0x8d + idx, exp.kelvin_bundle_tex_palette[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusTexColorKey : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1)) {
			warn(4);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				exp.kelvin_bundle_tex_color_key[idx] = val;
				pgraph_kelvin_bundle(&exp, 0x1c + idx, val, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexOffset : public SingleMthdTest {
	void adjust_orig_mthd() override {
		val &= ~0x7f;
		if (rnd() & 1) {
			val ^= 1 << (rnd() & 0x1f);
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x7f);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_offset[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x89 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 6, 2, 0);
			if (rnd() & 3)
				insrt(val, 15, 1, 0);
			if (rnd() & 3)
				insrt(val, 20, 4, 3);
			if (rnd() & 3)
				insrt(val, 24, 4, 3);
			if (rnd() & 3)
				insrt(val, 28, 4, 0);
			if (rnd() & 1)
				insrt(val, 24, 4, extr(val, 20, 4));
			if (rnd() & 3) {
				if (extr(val, 4, 2) == 2)
					insrt(val, 28, 4, 0);
				if (extr(val, 4, 2) == 1)
					insrt(val, 24, 8, 0);
			}
			if (rnd() & 1) {
				if (rnd() & 3)
					insrt(val, 2, 1, 0);
				if (rnd() & 3)
					insrt(val, 16, 4, 1);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		bool cube = extr(val, 2, 1);
		bool border = extr(val, 3, 1);
		int mode = extr(val, 4, 2);
		int fmt = extr(val, 8, 7);
		int mips = extr(val, 16, 4);
		int su = extr(val, 20, 4);
		int sv = extr(val, 24, 4);
		int sw = extr(val, 28, 4);
		bool rect = (fmt >= 0x10 && fmt <= 0x18) || (fmt >= 0x1b && fmt <= 0x26) || (fmt >= 0x2e && fmt <= 0x31) || (fmt >= 0x34 && fmt <= 0x37) || (fmt >= 0x3d && fmt <= 0x41);
		bool zcomp = (fmt >= 0x2a && fmt <= 0x31);
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (fmt >= 0x42)
			return false;
		switch (fmt) {
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0d:
			case 0x21:
			case 0x22:
			case 0x23:
				return false;
		}
		if (rect) {
			if (cube)
				return false;
			if (mips != 1)
				return false;
			if (mode == 3)
				return false;
		}
		if (cube) {
			if (mode != 2)
				return false;
			if (zcomp)
				return false;
			if (su != sv)
				return false;
		}
		if (mode == 0) {
			return false;
		} else if (mode == 1) {
			int max = border ? 0xc : 0xb;
			if (su > max)
				return false;
			if (sv || sw)
				return false;
		} else if (mode == 2) {
			int max = border ? 0xc : 0xb;
			if (su > max || sv > max)
				return false;
			if (sw)
				return false;
		} else if (mode == 3) {
			int max = border ? 0x9 : 0x8;
			if (su > max || sv > max || sw > max)
				return false;
			if (zcomp)
				return false;
		}
		if (extr(val, 6, 2))
			return false;
		if (extr(val, 15, 1))
			return false;
		if (mips > 0xd || mips == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			int mode = extr(val, 4, 2);
			int fmt = extr(val, 8, 7);
			bool zcomp = (fmt >= 0x2a && fmt <= 0x31);
			int mips = extr(val, 16, 4);
			int su = extr(val, 20, 4);
			int sv = extr(val, 24, 4);
			int sw = extr(val, 28, 4);
			if (mips > su && mips > sv && mips > sw)
				mips = std::max(su, std::max(sv, sw)) + 1;
			insrt(exp.kelvin_xf_mode_c[1 - (idx >> 1)], (idx & 1) * 16 + 2, 1, mode == 3 || zcomp);
			pgraph_kelvin_xf_mode(&exp);
			insrt(exp.kelvin_bundle_tex_format[idx], 1, 3, extr(val, 1, 3));
			insrt(exp.kelvin_bundle_tex_format[idx], 6, 2, mode);
			insrt(exp.kelvin_bundle_tex_format[idx], 8, 7, fmt);
			insrt(exp.kelvin_bundle_tex_format[idx], 16, 4, mips);
			insrt(exp.kelvin_bundle_tex_format[idx], 20, 4, su);
			insrt(exp.kelvin_bundle_tex_format[idx], 24, 4, sv);
			insrt(exp.kelvin_bundle_tex_format[idx], 28, 4, sw);
			pgraph_kelvin_bundle(&exp, 0x81 + idx, exp.kelvin_bundle_tex_format[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexWrap : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x01171717;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int mode_s = extr(val, 0, 3);
		int mode_t = extr(val, 8, 3);
		int mode_r = extr(val, 16, 3);
		if (val & ~0x01171717)
			return false;
		if (mode_s < 1 || mode_s > 5)
			return false;
		if (mode_t < 1 || mode_t > 5)
			return false;
		if (mode_r < 1 || mode_r > 5)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_wrap[idx] = val & 0x01171717;
			pgraph_kelvin_bundle(&exp, 0x6f + idx, exp.kelvin_bundle_tex_wrap[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexControl : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (extr(val, 31, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			insrt(exp.kelvin_xf_mode_c[1 - (idx >> 1)], (idx & 1) * 16, 1, extr(val, 30, 1));
			pgraph_kelvin_xf_mode(&exp);
			exp.kelvin_bundle_tex_control[idx] = val & 0x7fffffff;

			pgraph_kelvin_bundle(&exp, 0x73 + idx, exp.kelvin_bundle_tex_control[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexPitch : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xffff;
			if (!(rnd() & 3)) {
				val &= 0xe00f0000;
			}
			if (!(rnd() & 3))
				val = 0;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0xffff) && !!(val & 0xfff80000);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_pitch[idx] = val & 0xffff0000;
			pgraph_kelvin_bundle(&exp, 0x77 + idx, exp.kelvin_bundle_tex_pitch[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexFilter : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 13, 3, 1);
			if (rnd() & 3)
				insrt(val, 16, 6, 1);
			if (rnd() & 3)
				insrt(val, 22, 2, 0);
			if (rnd() & 3)
				insrt(val, 24, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int unk = extr(val, 13, 3);
		int min = extr(val, 16, 6);
		int mag = extr(val, 24, 4);
		if (extr(val, 22, 2))
			return false;
		if (unk != 1 && unk != 2)
			return false;
		if (min < 1 || min > 7)
			return false;
		if (mag != 1 && mag != 2 && mag != 4)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_filter[idx] = val & 0xff3fffff;
			pgraph_kelvin_bundle(&exp, 0x7d + idx, exp.kelvin_bundle_tex_filter[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexRect : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				val &= ~0xf000f000;
			}
			if (rnd() & 1) {
				if (rnd() & 1) {
					val &= ~0xffff;
				} else {
					val &= ~0xffff0000;
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (!extr(val, 0, 16) || extr(val, 0, 16) > 0x1000)
			return false;
		if (!extr(val, 16, 16) || extr(val, 16, 16) > 0x1000)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_rect[idx] = val & 0x1fff1fff;
			pgraph_kelvin_bundle(&exp, 0x85 + idx, exp.kelvin_bundle_tex_rect[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexPalette : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x32;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return !(val & 0x32);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_palette[idx] = val & ~0x32;
			pgraph_kelvin_bundle(&exp, 0x8d + idx, exp.kelvin_bundle_tex_palette[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexBorderColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_border_color[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x03 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk10 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_unk10[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x07 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk11 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_unk11[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x0a + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk12 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_unk12[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x0d + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk13 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (!(rnd() & 0xf))
			insrt(val, 0, 31, 0x40ffffe0 + (rnd() & 0x3f));
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return extr(val, 0, 31) <= 0x41000000;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_unk13[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x10 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk14 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_unk14[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x13 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexUnk15 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_unk15[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x16 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinTexColorKey : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_tex_color_key[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x1c + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcInAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_rc_in_alpha[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x30 + idx, exp.kelvin_bundle_rc_in_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcInColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_rc_in_color[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x40 + idx, exp.kelvin_bundle_rc_in_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			if (idx == 0)
				exp.kelvin_bundle_rc_factor_0[0] = val;
			else
				exp.kelvin_bundle_rc_factor_1[0] = val;
			exp.kelvin_bundle_rc_final_factor[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x20 + idx * 8, val, true);
			pgraph_kelvin_bundle(&exp, 0x6b + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 14))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_rc_out_alpha[idx] = val & 0x3cfff;
			pgraph_kelvin_bundle(&exp, 0x38 + idx, exp.kelvin_bundle_rc_out_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (!idx) {
			if (extr(val, 27, 3))
				return false;
		} else {
			int cnt = extr(val, 28, 2);
			if (cnt == 0 || cnt == 3)
				return false;
		}
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 9))
			return false;
		if (extr(val, 30, 2))
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval;
		rval = val & 0x3ffff;
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_rc_out_color[idx] = rval;
			pgraph_kelvin_bundle(&exp, 0x48 + idx, exp.kelvin_bundle_rc_out_color[idx], true);
			if (idx) {
				insrt(exp.kelvin_bundle_rc_config, 0, 4, extr(val, 28, 4));
				insrt(exp.kelvin_bundle_rc_config, 8, 1, extr(val, 27, 1));
				insrt(exp.kelvin_bundle_rc_config, 12, 1, 0);
				insrt(exp.kelvin_bundle_rc_config, 16, 1, 0);
				pgraph_kelvin_bundle(&exp, 0x50, exp.kelvin_bundle_rc_config, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFinal0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3f3f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c0c0)
			return false;
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_rc_final_a = val & 0x3f3f3f3f;
			pgraph_kelvin_bundle(&exp, 0x51, exp.kelvin_bundle_rc_final_a, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusRcFinal1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3fe0;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c01f)
			return false;
		for (int j = 1; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg == 0xa || reg == 0xb || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			exp.kelvin_bundle_rc_final_b = val & 0x3f3f3fe0;
			pgraph_kelvin_bundle(&exp, 0x52, exp.kelvin_bundle_rc_final_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcInAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_in_alpha[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x30 + idx, exp.kelvin_bundle_rc_in_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcInColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 16, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 24, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_in_color[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x40 + idx, exp.kelvin_bundle_rc_in_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFactor0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_factor_0[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x20 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFactor1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_factor_1[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x28 + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinalFactor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_final_factor[idx] = val;
			pgraph_kelvin_bundle(&exp, 0x6b + idx, val, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcOutAlpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 18, 14, 0);
			}
			if (rnd() & 1) {
				insrt(val, 28, 2, 1);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		if (extr(val, 12, 2))
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xa && reg != 0xb && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 18, 14))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_out_alpha[idx] = val & 0x3cfff;
			pgraph_kelvin_bundle(&exp, 0x38 + idx, exp.kelvin_bundle_rc_out_alpha[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcOutColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 1) {
				insrt(val, 20, 12, 0);
			}
			if (rnd() & 1) {
				insrt(val, 12, 2, 0);
			}
			if (rnd() & 1) {
				insrt(val, 0, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 4, 4, 0xd);
			}
			if (rnd() & 1) {
				insrt(val, 8, 4, 0xd);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		int op = extr(val, 15, 3);
		if (op == 5 || op == 7)
			return false;
		for (int j = 0; j < 3; j++) {
			int reg = extr(val, 4 * j, 4);
			if (reg != 0 && reg != 4 && reg != 5 && reg != 8 && reg != 9 && reg != 0xa && reg != 0xb && reg != 0xc && reg != 0xd)
				return false;
		}
		if (extr(val, 20, 12))
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval;
		rval = val & 0xfffff;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_out_color[idx] = rval;
			pgraph_kelvin_bundle(&exp, 0x48 + idx, exp.kelvin_bundle_rc_out_color[idx], true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1110f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & ~0x1110f)
			return false;
		if (extr(val, 0, 4) > 8)
			return false;
		if (extr(val, 0, 4) == 0)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_config = val & 0x1110f;
			pgraph_kelvin_bundle(&exp, 0x50, exp.kelvin_bundle_rc_config, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinal0 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3f3f;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c0c0)
			return false;
		for (int j = 0; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_final_a = val & 0x3f3f3f3f;
			pgraph_kelvin_bundle(&exp, 0x51, exp.kelvin_bundle_rc_final_a, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinRcFinal1 : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x3f3f3fe0;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xc0c0c01f)
			return false;
		for (int j = 1; j < 4; j++) {
			int reg = extr(val, 8 * j, 4);
			if (reg == 6 || reg == 7 || reg >= 0xe)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			exp.kelvin_bundle_rc_final_b = val & 0x3f3f3fe0;
			pgraph_kelvin_bundle(&exp, 0x52, exp.kelvin_bundle_rc_final_b, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xceeeeefe)
			return false;
		if (extr(val, 28, 2) && cls != 0x99)
			return false;
		if (extr(val, 8, 1) && !extr(val, 16, 1))
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.kelvin_bundle_config_a, 25, 1, extr(val, 0, 1));
			insrt(exp.kelvin_bundle_config_a, 23, 1, extr(val, 16, 1));
			insrt(exp.kelvin_bundle_config_a, 30, 2, 0);
			insrt(exp.kelvin_bundle_config_b, 2, 1, extr(val, 24, 1));
			insrt(exp.kelvin_bundle_config_b, 6, 1, extr(val, 20, 1));
			insrt(exp.kelvin_bundle_config_b, 10, 4, extr(val, 8, 4));
			insrt(exp.kelvin_bundle_raster, 29, 1, extr(val, 12, 1));
			pgraph_kelvin_bundle(&exp, 0x53, exp.kelvin_bundle_config_a, true);
			pgraph_kelvin_bundle(&exp, 0x56, exp.kelvin_bundle_config_b, true);
			pgraph_kelvin_bundle(&exp, 0x64, exp.kelvin_bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x00010007;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffefff8);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.kelvin_unkf5c, 2, 1, extr(val, 0, 1));
			insrt(exp.kelvin_xf_mode_b, 30, 1, extr(val, 16, 1));
			insrt(exp.kelvin_xf_mode_b, 28, 1, extr(val, 2, 1));
			insrt(exp.kelvin_xf_mode_b, 18, 1, extr(val, 1, 1));
			int spec = 0;
			if (extr(exp.kelvin_unkf5c, 3, 1))
				spec = extr(exp.kelvin_unkf5c, 2, 1) ? 2 : 1;
			insrt(exp.kelvin_xf_mode_b, 19, 2, spec);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (val & ~0xf) {
			warn(1);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.kelvin_unkf5c, 3, 1, extr(val, 3, 1));
				insrt(exp.kelvin_unkf5c, 21, 1, extr(val, 0, 1) && !extr(val, 1, 1));
				insrt(exp.kelvin_unkf5c, 22, 1, extr(val, 1, 1));
				int spec = 0;
				if (extr(exp.kelvin_unkf5c, 3, 1))
					spec = extr(exp.kelvin_unkf5c, 2, 1) ? 2 : 1;
				insrt(exp.kelvin_xf_mode_b, 19, 2, spec);
				insrt(exp.kelvin_xf_mode_b, 21, 2, extr(val, 2, 1));
				insrt(exp.kelvin_xf_mode_b, 23, 2, extr(val, 1, 1));
				insrt(exp.kelvin_xf_mode_b, 25, 2, extr(val, 0, 1) && !extr(val, 1, 1));
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.kelvin_xf_mode_a, 21, 1, rv & 1);
				pgraph_kelvin_xf_mode(&exp);
				insrt(exp.kelvin_bundle_config_b, 16, 3, rv);
				pgraph_kelvin_bundle(&exp, 0x56, exp.kelvin_bundle_config_b, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 3)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				int rval = val;
				if (val == 0)
					rval = 4;
				insrt(exp.kelvin_xf_mode_a, 22, 3, rval);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.kelvin_bundle_config_b, 8, 1, val);
				pgraph_kelvin_bundle(&exp, 0x56, exp.kelvin_bundle_config_b, true);
				insrt(exp.kelvin_xf_mode_a, 19, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusFogColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1)) {
			warn(4);
		} else {
			if (!extr(exp.nsource, 1, 1)) {
				insrt(exp.kelvin_bundle_fog_color, 0, 8, extr(val, 16, 8));
				insrt(exp.kelvin_bundle_fog_color, 8, 8, extr(val, 8, 8));
				insrt(exp.kelvin_bundle_fog_color, 16, 8, extr(val, 0, 8));
				insrt(exp.kelvin_bundle_fog_color, 24, 8, extr(val, 24, 8));
				pgraph_kelvin_bundle(&exp, 0x60, exp.kelvin_bundle_fog_color, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinConfig : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x31111101;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		if (val & 0xceeeeffe)
			return false;
		if (extr(val, 28, 2) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			insrt(exp.kelvin_bundle_config_a, 25, 1, extr(val, 0, 1));
			insrt(exp.kelvin_bundle_config_a, 23, 1, extr(val, 16, 1));
			insrt(exp.kelvin_bundle_config_a, 30, 2, extr(val, 28, 2));
			insrt(exp.kelvin_bundle_config_b, 2, 1, extr(val, 24, 1));
			insrt(exp.kelvin_bundle_config_b, 6, 1, extr(val, 20, 1));
			insrt(exp.kelvin_bundle_raster, 29, 1, extr(val, 12, 1));
			pgraph_kelvin_bundle(&exp, 0x53, exp.kelvin_bundle_config_a, true);
			pgraph_kelvin_bundle(&exp, 0x56, exp.kelvin_bundle_config_b, true);
			pgraph_kelvin_bundle(&exp, 0x64, exp.kelvin_bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightModel : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x00030001;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xfffcfffe);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			insrt(exp.kelvin_xf_mode_b, 30, 1, extr(val, 16, 1));
			insrt(exp.kelvin_xf_mode_b, 17, 1, extr(val, 17, 1));
			insrt(exp.kelvin_xf_mode_b, 18, 1, extr(val, 0, 1));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinLightMaterial : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0x1ffff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & ~0x1ffff)
			return false;
		for (int i = 0; i < 8; i++)
			if (extr(val, i*2, 2) == 3)
				return false;
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			insrt(exp.kelvin_xf_mode_b, 0, 2, extr(val, 14, 2));
			insrt(exp.kelvin_xf_mode_b, 2, 2, extr(val, 12, 2));
			insrt(exp.kelvin_xf_mode_b, 4, 2, extr(val, 10, 2));
			insrt(exp.kelvin_xf_mode_b, 6, 2, extr(val, 8, 2));
			insrt(exp.kelvin_xf_mode_b, 19, 2, extr(val, 6, 2));
			insrt(exp.kelvin_xf_mode_b, 21, 2, extr(val, 4, 2));
			insrt(exp.kelvin_xf_mode_b, 23, 2, extr(val, 2, 2));
			insrt(exp.kelvin_xf_mode_b, 25, 2, extr(val, 0, 2));
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
				if (rnd() & 1) {
					val |= (rnd() & 1 ? 0x800 : 0x2600);
				}
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		uint32_t err = 0;
		uint32_t rv = 0;
		if (val == 0x800) {
			rv = 1;
		} else if (val == 0x801) {
			rv = 3;
		} else if (val == 0x802) {
			rv = 5;
		} else if (val == 0x803) {
			rv = 7;
		} else if (val == 0x804) {
			rv = 4;
		} else if (val == 0x2601) {
			rv = 0;
		} else {
			err |= 1;
		}
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (extr(exp.kelvin_unkf5c, 0, 1))
				nv04_pgraph_blowup(&exp, 0x40000);
			if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
				insrt(exp.kelvin_xf_mode_a, 21, 1, rv & 1);
				pgraph_kelvin_xf_mode(&exp);
				insrt(exp.kelvin_bundle_config_b, 16, 3, rv);
				pgraph_kelvin_bundle(&exp, 0x56, exp.kelvin_bundle_config_b, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogCoord : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() {
		return val < 4 || val == 6;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			int rval = val & 7;
			if (rval == 6)
				rval = 4;
			insrt(exp.kelvin_xf_mode_a, 22, 3, rval);
			pgraph_kelvin_xf_mode(&exp);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogEnable : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xffff;
			if (rnd() & 1) {
				val &= 0xf;
			}
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		uint32_t err = 0;
		if (val > 1)
			err |= 1;
		if (extr(exp.kelvin_unkf5c, 0, 1))
			err |= 4;
		if (err) {
			warn(err);
		} else {
			if (extr(exp.kelvin_unkf5c, 0, 1))
				nv04_pgraph_blowup(&exp, 0x40000);
			if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
				insrt(exp.kelvin_bundle_config_b, 8, 1, val);
				pgraph_kelvin_bundle(&exp, 0x56, exp.kelvin_bundle_config_b, true);
				insrt(exp.kelvin_xf_mode_a, 19, 1, val);
				pgraph_kelvin_xf_mode(&exp);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinFogColor : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
	}
	bool can_warn() override {
		return true;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 0, 1))
			nv04_pgraph_blowup(&exp, 0x40000);
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			insrt(exp.kelvin_bundle_fog_color, 0, 8, extr(val, 16, 8));
			insrt(exp.kelvin_bundle_fog_color, 8, 8, extr(val, 8, 8));
			insrt(exp.kelvin_bundle_fog_color, 16, 8, extr(val, 0, 8));
			insrt(exp.kelvin_bundle_fog_color, 24, 8, extr(val, 24, 8));
			pgraph_kelvin_bundle(&exp, 0x60, exp.kelvin_bundle_fog_color, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusClipRectMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			insrt(exp.kelvin_bundle_raster, 31, 1, val);
			pgraph_kelvin_bundle(&exp, 0x64, exp.kelvin_bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusClipRectHoriz : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			uint32_t rval = (val & 0x0fff0fff) ^ 0x08000800;
			for (int i = idx; i < 8; i++) {
				exp.kelvin_bundle_clip_rect_horiz[i] = rval;
				pgraph_kelvin_bundle(&exp, 0x91 + i, rval, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuCelsiusClipRectVert : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extrs(val, 0, 12) <= extrs(val, 16, 12);
	}
	void emulate_mthd() override {
		if (!extr(exp.nsource, 1, 1)) {
			uint32_t rval = (val & 0x0fff0fff) ^ 0x08000800;
			for (int i = idx; i < 8; i++) {
				exp.kelvin_bundle_clip_rect_vert[i] = rval;
				pgraph_kelvin_bundle(&exp, 0x99 + i, rval, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectMode : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xf;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
		adjust_orig_bundle(&orig);
	}
	bool is_valid_val() override {
		return val < 2;
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			insrt(exp.kelvin_bundle_raster, 31, 1, val);
			pgraph_kelvin_bundle(&exp, 0x64, exp.kelvin_bundle_raster, true);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectHoriz : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extr(val, 0, 12) <= extr(val, 16, 12);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			uint32_t rval = val & 0x0fff0fff;
			for (int i = idx; i < 8; i++) {
				exp.kelvin_bundle_clip_rect_horiz[i] = rval;
				pgraph_kelvin_bundle(&exp, 0x91 + i, rval, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdKelvinClipRectVert : public SingleMthdTest {
	void adjust_orig_mthd() override {
		adjust_orig_bundle(&orig);
		if (rnd() & 1) {
			val &= 0xffff;
			val *= 0x00010001;
		}
		if (rnd() & 1) {
			val &= 0x0fff0fff;
			if (rnd() & 1) {
				val |= 1 << (rnd() & 0x1f);
				if (rnd() & 1) {
					val |= 1 << (rnd() & 0x1f);
				}
			}
		}
	}
	bool is_valid_val() override {
		return !(val & 0xf000f000) && extr(val, 0, 12) <= extr(val, 16, 12);
	}
	void emulate_mthd() override {
		if (extr(exp.kelvin_unkf5c, 4, 1) && extr(exp.debug[3], 3, 1))
			nv04_pgraph_blowup(&exp, 0x80000);
		if (!extr(exp.nsource, 1, 1) && !extr(exp.nsource, 18, 1) && !extr(exp.nsource, 19, 1)) {
			uint32_t rval = val & 0x0fff0fff;
			for (int i = idx; i < 8; i++) {
				exp.kelvin_bundle_clip_rect_vert[i] = rval;
				pgraph_kelvin_bundle(&exp, 0x99 + i, rval, true);
			}
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

std::vector<SingleMthdTest *> EmuCelsius::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", 0, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", 2, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", 1, cls, 0x110),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", 3, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_tex_a", 4, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dma_tex_b", 5, cls, 0x188), // XXX
		new UntestedMthd(opt, rnd(), "dma_vtx", 6, cls, 0x18c), // XXX
		new UntestedMthd(opt, rnd(), "dma_state", 7, cls, 0x190), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", 8, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", 9, cls, 0x198, 3, SURF_NV10),
		new MthdEmuCelsiusClip(opt, rnd(), "clip_h", 10, cls, 0x200, 0),
		new MthdEmuCelsiusClip(opt, rnd(), "clip_v", 10, cls, 0x204, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", 11, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", 12, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", 13, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", 14, cls, 0x214, 3, SURF_NV10),
		new MthdEmuCelsiusTexOffset(opt, rnd(), "tex_offset", 15, cls, 0x218, 2),
		new MthdEmuCelsiusTexFormat(opt, rnd(), "tex_format", 16, cls, 0x220, 2),
		new MthdEmuCelsiusTexControl(opt, rnd(), "tex_control", 17, cls, 0x228, 2),
		new MthdEmuCelsiusTexPitch(opt, rnd(), "tex_pitch", 18, cls, 0x230, 2),
		new MthdEmuCelsiusTexUnk238(opt, rnd(), "tex_unk238", 19, cls, 0x238, 2),
		new MthdEmuCelsiusTexRect(opt, rnd(), "tex_rect", 20, cls, 0x240, 2),
		new MthdEmuCelsiusTexFilter(opt, rnd(), "tex_filter", 21, cls, 0x248, 2),
		new MthdEmuCelsiusTexPalette(opt, rnd(), "tex_palette", 22, cls, 0x250, 2),
		new MthdEmuCelsiusRcInAlpha(opt, rnd(), "rc_in_alpha", 23, cls, 0x260, 2),
		new MthdEmuCelsiusRcInColor(opt, rnd(), "rc_in_color", 24, cls, 0x268, 2),
		new MthdEmuCelsiusRcFactor(opt, rnd(), "rc_factor", 25, cls, 0x270, 2),
		new MthdEmuCelsiusRcOutAlpha(opt, rnd(), "rc_out_alpha", 26, cls, 0x278, 2),
		new MthdEmuCelsiusRcOutColor(opt, rnd(), "rc_out_color", 27, cls, 0x280, 2),
		new MthdEmuCelsiusRcFinal0(opt, rnd(), "rc_final_0", 28, cls, 0x288),
		new MthdEmuCelsiusRcFinal1(opt, rnd(), "rc_final_1", 29, cls, 0x28c),
		new MthdEmuCelsiusConfig(opt, rnd(), "config", 30, cls, 0x290),
		new MthdEmuCelsiusLightModel(opt, rnd(), "light_model", 31, cls, 0x294),
		new MthdEmuCelsiusLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdEmuCelsiusFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdEmuCelsiusFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdEmuCelsiusFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdEmuCelsiusFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdEmuCelsiusTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x2ac, 2),
		new MthdEmuCelsiusClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdEmuCelsiusClipRectHoriz(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8),
		new MthdEmuCelsiusClipRectVert(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x400), // XXX
	};
	if (cls == 0x56) {
	} else {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "unk114", -1, cls, 0x114), // XXX
			new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
			new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
			new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
			new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
			new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Kelvin::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_tex_a", -1, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dma_tex_b", -1, cls, 0x188), // XXX
		new UntestedMthd(opt, rnd(), "dma_state", -1, cls, 0x190), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c), // XXX
		new UntestedMthd(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0), // XXX
		new UntestedMthd(opt, rnd(), "dma_fence", -1, cls, 0x1a4), // XXX
		new UntestedMthd(opt, rnd(), "dma_query", -1, cls, 0x1a8), // XXX
		new MthdKelvinClip(opt, rnd(), "clip_h", -1, cls, 0x200, 0),
		new MthdKelvinClip(opt, rnd(), "clip_v", -1, cls, 0x204, 1),
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new MthdKelvinRcInAlpha(opt, rnd(), "rc_in_alpha", -1, cls, 0x260, 8),
		new MthdKelvinRcFinal0(opt, rnd(), "rc_final_0", -1, cls, 0x288),
		new MthdKelvinRcFinal1(opt, rnd(), "rc_final_1", -1, cls, 0x28c),
		new MthdKelvinConfig(opt, rnd(), "config", -1, cls, 0x290),
		new MthdKelvinLightModel(opt, rnd(), "light_model", -1, cls, 0x294),
		new MthdKelvinLightMaterial(opt, rnd(), "light_material", -1, cls, 0x298),
		new MthdKelvinFogMode(opt, rnd(), "fog_mode", -1, cls, 0x29c),
		new MthdKelvinFogCoord(opt, rnd(), "fog_coord", -1, cls, 0x2a0),
		new MthdKelvinFogEnable(opt, rnd(), "fog_enable", -1, cls, 0x2a4),
		new MthdKelvinFogColor(opt, rnd(), "fog_color", -1, cls, 0x2a8),
		new MthdKelvinClipRectMode(opt, rnd(), "clip_rect_mode", -1, cls, 0x2b4),
		new MthdKelvinClipRectHoriz(opt, rnd(), "clip_rect_horiz", -1, cls, 0x2c0, 8),
		new MthdKelvinClipRectVert(opt, rnd(), "clip_rect_vert", -1, cls, 0x2e0, 8),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xa00, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xa40, 0x8), // XXX
		new MthdKelvinRcFactor0(opt, rnd(), "rc_factor_0", -1, cls, 0xa60, 8),
		new MthdKelvinRcFactor1(opt, rnd(), "rc_factor_1", -1, cls, 0xa80, 8),
		new MthdKelvinRcOutAlpha(opt, rnd(), "rc_out_alpha", -1, cls, 0xaa0, 8),
		new MthdKelvinRcInColor(opt, rnd(), "rc_in_color", -1, cls, 0xac0, 8),
		new MthdKelvinTexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0xae0, 4),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xaf0, 4), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xb00, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0xc00, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1800, 0x80), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1a00, 0x40), // XXX
		new MthdKelvinTexOffset(opt, rnd(), "tex_offset", -1, cls, 0x1b00, 4, 0x40),
		new MthdKelvinTexFormat(opt, rnd(), "tex_format", -1, cls, 0x1b04, 4, 0x40),
		new MthdKelvinTexWrap(opt, rnd(), "tex_wrap", -1, cls, 0x1b08, 4, 0x40),
		new MthdKelvinTexControl(opt, rnd(), "tex_control", -1, cls, 0x1b0c, 4, 0x40),
		new MthdKelvinTexPitch(opt, rnd(), "tex_pitch", -1, cls, 0x1b10, 4, 0x40),
		new MthdKelvinTexFilter(opt, rnd(), "tex_filter", -1, cls, 0x1b14, 4, 0x40),
		new MthdKelvinTexRect(opt, rnd(), "tex_rect", -1, cls, 0x1b1c, 4, 0x40),
		new MthdKelvinTexPalette(opt, rnd(), "tex_palette", -1, cls, 0x1b20, 4, 0x40),
		new MthdKelvinTexBorderColor(opt, rnd(), "tex_border_color", -1, cls, 0x1b24, 4, 0x40),
		new MthdKelvinTexUnk10(opt, rnd(), "tex_unk10", -1, cls, 0x1b68, 3, 0x40),
		new MthdKelvinTexUnk11(opt, rnd(), "tex_unk11", -1, cls, 0x1b6c, 3, 0x40),
		new MthdKelvinTexUnk12(opt, rnd(), "tex_unk12", -1, cls, 0x1b70, 3, 0x40),
		new MthdKelvinTexUnk13(opt, rnd(), "tex_unk13", -1, cls, 0x1b74, 3, 0x40),
		new MthdKelvinTexUnk14(opt, rnd(), "tex_unk14", -1, cls, 0x1b78, 3, 0x40),
		new MthdKelvinTexUnk15(opt, rnd(), "tex_unk15", -1, cls, 0x1b7c, 3, 0x40),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1d60, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1d80, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1dc0, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e00, 8), // XXX
		new MthdKelvinRcFinalFactor(opt, rnd(), "rc_final_factor", -1, cls, 0x1e20, 2),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e28, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e30, 4), // XXX
		new MthdKelvinRcOutColor(opt, rnd(), "rc_out_color", -1, cls, 0x1e40, 8),
		new MthdKelvinRcConfig(opt, rnd(), "rc_config", -1, cls, 0x1e60),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e68, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e70, 4), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1e80, 0x10), // XXX
	};
	if (cls == 0x597) {
		res.insert(res.end(), {
			new UntestedMthd(opt, rnd(), "dma_clipid", -1, cls, 0x1ac), // XXX
			new UntestedMthd(opt, rnd(), "dma_zcull", -1, cls, 0x1b0), // XXX
		});
	}
	return res;
}

std::vector<SingleMthdTest *> Rankine::mthds() {
	std::vector<SingleMthdTest *> res = {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdWarning(opt, rnd(), "warning", -1, cls, 0x108),
		new MthdState(opt, rnd(), "state", -1, cls, 0x10c),
		new MthdSync(opt, rnd(), "sync", -1, cls, 0x110),
		new MthdFlipSet(opt, rnd(), "flip_write", -1, cls, 0x120, 1, 1),
		new MthdFlipSet(opt, rnd(), "flip_read", -1, cls, 0x124, 1, 0),
		new MthdFlipSet(opt, rnd(), "flip_modulo", -1, cls, 0x128, 1, 2),
		new MthdFlipBumpWrite(opt, rnd(), "flip_bump_write", -1, cls, 0x12c, 1),
		new UntestedMthd(opt, rnd(), "flip_unk130", -1, cls, 0x130),
		new MthdPmTrigger(opt, rnd(), "pm_trigger", -1, cls, 0x140),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new UntestedMthd(opt, rnd(), "dma_tex_a", -1, cls, 0x184), // XXX
		new UntestedMthd(opt, rnd(), "dma_tex_b", -1, cls, 0x188), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_b", -1, cls, 0x18c, 6, SURF_NV10),
		new UntestedMthd(opt, rnd(), "dma_state", -1, cls, 0x190), // XXX
		new MthdDmaSurf(opt, rnd(), "dma_surf_color_a", -1, cls, 0x194, 2, SURF_NV10),
		new MthdDmaSurf(opt, rnd(), "dma_surf_zeta", -1, cls, 0x198, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "dma_vtx_a", -1, cls, 0x19c), // XXX
		new UntestedMthd(opt, rnd(), "dma_vtx_b", -1, cls, 0x1a0), // XXX
		new UntestedMthd(opt, rnd(), "dma_fence", -1, cls, 0x1a4), // XXX
		new UntestedMthd(opt, rnd(), "dma_query", -1, cls, 0x1a8), // XXX
		new UntestedMthd(opt, rnd(), "dma_clipid", -1, cls, 0x1ac), // XXX
		new UntestedMthd(opt, rnd(), "dma_zcull", -1, cls, 0x1b0), // XXX
		new UntestedMthd(opt, rnd(), "clip_h", -1, cls, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "clip_v", -1, cls, 0x204), // XXX
		new MthdSurf3DFormat(opt, rnd(), "surf_format", -1, cls, 0x208, true),
		new MthdSurfPitch2(opt, rnd(), "surf_pitch_2", -1, cls, 0x20c, 2, 3, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "color_offset", -1, cls, 0x210, 2, SURF_NV10),
		new MthdSurfOffset(opt, rnd(), "zeta_offset", -1, cls, 0x214, 3, SURF_NV10),
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x218, 2), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x220, 8), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x240, 0x10), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x280, 0x20), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x300, 0x40), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x400, 0x100), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x800, 0x200), // XXX
		new UntestedMthd(opt, rnd(), "meh", -1, cls, 0x1000, 0x400), // XXX
	};
	return res;
}

}
}
