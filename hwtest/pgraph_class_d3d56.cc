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

class MthdD3D56TexColorKey : public SingleMthdTest {
	void emulate_mthd() override {
		exp.misc32[2] = val;
		insrt(exp.valid[1], 12, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TexOffset : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		return (val & 0xff) == 0 || cls == 0x48;
	}
	void emulate_mthd() override {
		if (which & 1) {
			exp.misc32[0] = val;
			insrt(exp.valid[1], 14, 1, 1);
		}
		if (which & 2) {
			exp.misc32[1] = val;
			insrt(exp.valid[1], 22, 1, 1);
		}
	}
public:
	MthdD3D56TexOffset(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdD3D56TexFormat : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 2, 1);
			if (rnd() & 3)
				insrt(val, 2, 2, 0);
			if (rnd() & 3)
				insrt(val, 4, 2, 1);
			if (rnd() & 3)
				insrt(val, 6, 2, 1);
			if (rnd() & 3)
				insrt(val, 11, 1, 0);
			if (rnd() & 3)
				insrt(val, 24, 3, 1);
			if (rnd() & 3)
				insrt(val, 28, 3, 1);
		}
	}
	bool is_valid_val() override {
		if (!extr(val, 0, 2) || extr(val, 0, 2) == 3)
			return false;
		if (!extr(val, 4, 2) || extr(val, 4, 2) == 3)
			return false;
		if (!extr(val, 6, 2) || extr(val, 6, 2) == 3)
			return false;
		if (!extr(val, 8, 3))
			return false;
		if (extr(val, 11, 1))
			return false;
		if (!extr(val, 12, 4))
			return false;
		if (extr(val, 16, 4) > 0xb)
			return false;
		if (extr(val, 20, 4) > 0xb)
			return false;
		if (extr(val, 24, 3) < 1 || extr(val, 24, 3) > 4)
			return false;
		if (extr(val, 28, 3) < 1 || extr(val, 28, 3) > 4)
			return false;
		if (cls == 0x54) {
			if (extr(val, 2, 2) > 1)
				return false;
		} else {
			if (extr(val, 2, 2))
				return false;;
		}
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval = val & 0xfffff7a6;
		if (cls == 0x54 && extr(val, 8, 3) == 1)
			insrt(rval, 8, 3, 0);
		if (which & 1) {
			insrt(exp.valid[1], 15, 1, 1);
			exp.d3d56_tex_format[0] = rval;
		}
		if (which & 2) {
			insrt(exp.valid[1], 21, 1, 1);
			exp.d3d56_tex_format[1] = rval;
		}
	}
public:
	MthdD3D56TexFormat(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdD3D56TexFilter : public SingleMthdTest {
	int which;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 5, 3, 0);
			if (rnd() & 3)
				insrt(val, 13, 2, 0);
		}
	}
	bool is_valid_val() override {
		if (extr(val, 13, 2))
			return false;
		if (extr(val, 5, 3))
			return false;
		if (extr(val, 24, 3) == 0 || extr(val, 24, 3) > 6)
			return false;
		if (extr(val, 28, 3) == 0 || extr(val, 28, 3) > 6)
			return false;
		return true;
	}
	void emulate_mthd() override {
		bool is_mip = extr(val, 24, 3) == 5 || extr(val, 24, 3) == 6;
		uint32_t rval = val & 0xffff9e1e;
		if (which & 1) {
			insrt(exp.valid[1], 16, 1, 1);
			exp.d3d56_tex_filter[0] = rval;
		}
		if (cls == 0x54 && is_mip && !extr(val, 15, 1)) {
			int bias = extr(val, 16, 8);
			if (bias < 0x78 || bias >= 0x80)
				bias += 8;
			insrt(rval, 16, 8, bias);
		}
		if (which & 2) {
			insrt(exp.valid[1], 23, 1, 1);
			exp.d3d56_tex_filter[1] = rval;
		}
	}
public:
	MthdD3D56TexFilter(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which) {}
};

class MthdD3D6CombineControl : public SingleMthdTest {
	int which, ac;
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0x00e0e0e0;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (val & 0x00e0e0e0)
			return false;
		if (!extr(val, 2, 3))
			return false;
		if (!extr(val, 10, 3))
			return false;
		if (!extr(val, 18, 3))
			return false;
		if (!extr(val, 26, 3))
			return false;
		if (!extr(val, 29, 3))
			return false;
		if (which) {
			if (extr(val, 2, 3) == 7)
				return false;
			if (extr(val, 10, 3) == 7)
				return false;
			if (extr(val, 18, 3) == 7)
				return false;
			if (extr(val, 26, 3) == 7)
				return false;
		}
		return true;
	}
	void emulate_mthd() override {
		if (ac)
			exp.d3d56_rc_color[which] = val & 0xff1f1f1f;
		else
			exp.d3d56_rc_alpha[which] = val & 0xfd1d1d1d;
		insrt(exp.valid[1], 28 - which * 2 - ac, 1, 1);
	}
public:
	MthdD3D6CombineControl(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, int which, int ac)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd), which(which), ac(ac) {}
};

class MthdD3D6CombineFactor : public SingleMthdTest {
	void emulate_mthd() override {
		exp.misc32[3] = val;
		insrt(exp.valid[1], 24, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56Blend : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 9, 3, 0);
			if (rnd() & 3)
				insrt(val, 13, 3, 0);
			if (rnd() & 3)
				insrt(val, 17, 3, 0);
			if (rnd() & 3)
				insrt(val, 21, 3, 0);
			if (rnd() & 1)
				insrt(val, 0, 4, 0);
		}
	}
	bool is_valid_val() override {
		if (cls == 0x54) {
			if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
				return false;
		} else {
			if (extr(val, 0, 4))
				return false;
		}
		if (extr(val, 4, 2) < 1 || extr(val, 4, 2) > 2)
			return false;
		if (!extr(val, 6, 2))
			return false;
		if (extr(val, 9, 3))
			return false;
		if (extr(val, 13, 3))
			return false;
		if (extr(val, 17, 3))
			return false;
		if (extr(val, 21, 3))
			return false;
		if (extr(val, 24, 4) < 1 || extr(val, 24, 4) > 0xb)
			return false;
		if (extr(val, 28, 4) < 1 || extr(val, 28, 4) > 0xb)
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval = val & 0xff1111ff;
		if (cls == 0x55)
			insrt(rval, 0, 4, 0);
		exp.d3d56_blend = rval;
		insrt(exp.valid[1], 20, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56Config : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 8, 4, 1);
			if (rnd() & 3)
				insrt(val, 15, 1, 0);
			if (rnd() & 3)
				insrt(val, 16, 4, 1);
			if (rnd() & 3)
				insrt(val, 25, 5, 0);
		}
	}
	bool is_valid_val() override {
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			return false;
		if (extr(val, 15, 1))
			return false;
		if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
			return false;
		if (extr(val, 20, 2) < 1)
			return false;
		if (extr(val, 25, 5) && cls == 0x54)
			return false;
		if (extr(val, 30, 2) < 1 || extr(val, 30, 2) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		uint32_t rval = val & 0xffff5fff;
		if (cls == 0x54)
			insrt(rval, 25, 5, 0x1e);
		exp.d3d56_config = rval;
		insrt(exp.dvd_format, 0, 8, extr(val, 13, 1));
		insrt(exp.valid[1], 17, 1, 1);
		if (cls == 0x54) {
			exp.d3d56_stencil_func = 0x80;
			exp.d3d56_stencil_op = 0x222;
			insrt(exp.valid[1], 18, 1, 1);
			insrt(exp.valid[1], 19, 1, 1);
		}
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D6StencilFunc : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1)
			val &= ~0xe;
	}
	bool is_valid_val() override {
		if (extr(val, 1, 3))
			return false;
		if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.d3d56_stencil_func = val & 0xfffffff1;
		insrt(exp.valid[1], 18, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D6StencilOp : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= ~0xfffff000;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (extr(val, 0, 4) < 1 || extr(val, 0, 4) > 8)
			return false;
		if (extr(val, 4, 4) < 1 || extr(val, 4, 4) > 8)
			return false;
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			return false;
		if (extr(val, 12, 20))
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.d3d56_stencil_op = val & 0xfff;
		insrt(exp.valid[1], 19, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56FogColor : public SingleMthdTest {
	void emulate_mthd() override {
		exp.misc24[0] = val & 0xffffff;
		insrt(exp.valid[1], 13, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuD3D0TexFormat : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			val &= 0xff31ffff;
			val ^= 1 << (rnd() & 0x1f);
		}
	}
	bool is_valid_val() override {
		if (val & ~0xff31ffff)
			return false;
		int max_l = extr(val, 28, 4);
		int min_l = extr(val, 24, 4);
		if (min_l < 2)
			return false;
		if (min_l > max_l)
			return false;
		if (max_l > 0xb)
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.misc32[2] = val;
		int fmt = 5;
		int sfmt = extr(val, 20, 4);
		if (sfmt == 0)
			fmt = 2;
		else if (sfmt == 1)
			fmt = 3;
		else if (sfmt == 2)
			fmt = 4;
		else if (sfmt == 3)
			fmt = 5;
		int max_l = extr(val, 28, 4);
		int min_l = extr(val, 24, 4);
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d56_tex_format[i], 1, 1, 0);
			insrt(exp.d3d56_tex_format[i], 2, 1, extr(val, 16, 1));
			insrt(exp.d3d56_tex_format[i], 7, 1, 0);
			insrt(exp.d3d56_tex_format[i], 8, 3, fmt);
			insrt(exp.d3d56_tex_format[i], 12, 4, max_l - min_l + 1);
			insrt(exp.d3d56_tex_format[i], 16, 4, max_l);
			insrt(exp.d3d56_tex_format[i], 20, 4, max_l);
		}
		insrt(exp.valid[1], 21, 1, 1);
		insrt(exp.valid[1], 15, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuD3D0TexFilter : public SingleMthdTest {
	void emulate_mthd() override {
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d56_tex_filter[i], 1, 4, extr(val, 1, 4));
			insrt(exp.d3d56_tex_filter[i], 9, 4, extr(val, 9, 4));
			insrt(exp.d3d56_tex_filter[i], 15, 1, 0);
			insrt(exp.d3d56_tex_filter[i], 16, 8, extrs(val, 17, 7));
			insrt(exp.d3d56_tex_filter[i], 27, 1, 1);
			insrt(exp.d3d56_tex_filter[i], 31, 1, 1);
		}
		insrt(exp.valid[1], 23, 1, 1);
		insrt(exp.valid[1], 16, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuD3D0Config : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 0, 4, 1);
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (extr(val, 9, 1))
			return false;
		if (!extr(val, 12, 2))
			return false;
		if (extr(val, 14, 1))
			return false;
		if (extr(val, 16, 4) < 1 || extr(val, 16, 4) > 8)
			return false;
		if (extr(val, 20, 4) > 4)
			return false;
		if (extr(val, 24, 4) > 4)
			return false;
		if (extr(val, 0, 4) > 2)
			return false;
		return true;
	}
	void emulate_mthd() override {
		exp.misc32[3] = val;
		int filt = 1;
		int origin = 0;
		int sinterp = extr(val, 0, 4);
		if (sinterp == 0)
			origin = 2;
		if (sinterp == 2)
			filt = 2;
		int wrapu = extr(val, 4, 2);
		if (wrapu == 0)
			wrapu = 9;
		int wrapv = extr(val, 6, 2);
		if (wrapv == 0)
			wrapv = 9;
		int scull = extr(val, 12, 2);
		int cull = 0;
		if (scull == 0) {
			cull = 1;
		} else if (scull == 1) {
			cull = 1;
		} else if (scull == 2) {
			cull = 3;
		} else if (scull == 3) {
			cull = 2;
		}
		int dblend = 6;
		int sblend = 5;
		if (extr(val, 29, 1)) {
			dblend = 0xa;
			sblend = 0x9;
		}
		if (extr(val, 28, 1))
			dblend = sblend = 2;
		if (extr(val, 30, 1))
			dblend = 1;
		if (extr(val, 31, 1))
			sblend = 1;
		for (int i = 0; i < 2; i++) {
			insrt(exp.d3d56_tex_format[i], 4, 2, origin);
			insrt(exp.d3d56_tex_format[i], 24, 4, wrapu);
			insrt(exp.d3d56_tex_format[i], 28, 4, wrapv);
			insrt(exp.d3d56_tex_filter[i], 24, 3, filt);
			insrt(exp.d3d56_tex_filter[i], 28, 3, filt);
		}
		insrt(exp.d3d56_blend, 4, 2, 1);
		insrt(exp.d3d56_blend, 6, 2, 2);
		insrt(exp.d3d56_blend, 8, 1, 1);
		insrt(exp.d3d56_blend, 12, 1, 0);
		insrt(exp.d3d56_blend, 16, 1, 1);
		insrt(exp.d3d56_blend, 20, 1, 1);
		insrt(exp.d3d56_blend, 24, 4, sblend);
		insrt(exp.d3d56_blend, 28, 4, dblend);
		insrt(exp.d3d56_config, 14, 1, !!extr(val, 20, 3));
		insrt(exp.d3d56_config, 16, 4, extr(val, 16, 4));
		insrt(exp.d3d56_config, 20, 2, cull);
		insrt(exp.d3d56_config, 22, 1, 1);
		insrt(exp.d3d56_config, 23, 1, extr(val, 15, 1));
		insrt(exp.d3d56_config, 24, 1, !!extr(val, 20, 3));
		insrt(exp.d3d56_config, 25, 1, 0);
		insrt(exp.d3d56_config, 26, 1, 0);
		insrt(exp.d3d56_config, 27, 3, extr(val, 24, 3) ? 0x7 : 0);
		insrt(exp.d3d56_config, 30, 2, 1);
		exp.d3d56_stencil_func = 0x80;
		exp.d3d56_stencil_op = 0x222;
		insrt(exp.valid[1], 17, 1, 1);
		insrt(exp.valid[1], 24, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuD3D0Alpha : public SingleMthdTest {
	void adjust_orig_mthd() override {
		if (rnd() & 1) {
			if (rnd() & 3)
				insrt(val, 8, 4, 1);
			val &= ~0xfffff000;
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
			if (rnd() & 1) {
				val ^= 1 << (rnd() & 0x1f);
			}
		}
	}
	bool is_valid_val() override {
		if (extr(val, 8, 4) < 1 || extr(val, 8, 4) > 8)
			return false;
		if (extr(val, 12, 20))
			return false;
		return true;
	}
	void emulate_mthd() override {
		insrt(exp.d3d56_config, 0, 12, val);
		insrt(exp.d3d56_config, 12, 1, 1);
		insrt(exp.valid[1], 18, 1, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvFogCol1 : public SingleMthdTest {
	void emulate_mthd() override {
		exp.d3d56_tlv_fog_tri_col1 = val;
		insrt(exp.valid[1], 0, 1, 1);
		insrt(exp.valid[0], idx, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvColor : public SingleMthdTest {
	void emulate_mthd() override {
		exp.d3d56_tlv_color = val;
		insrt(exp.valid[1], 1, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvX : public SingleMthdTest {
	void emulate_mthd() override {
		uint16_t tv = nv03_pgraph_convert_xy(val);
		if (cls != 0x48 && extr(exp.debug[0], 14, 1) && exp.dvd_format & 1) {
			if (tv >= 0x8000 && tv < 0x8008)
				tv = 0x8000;
			else
				tv -= 8;
		}
		insrt(exp.d3d56_tlv_xy, 0, 16, tv);
		insrt(exp.valid[1], 5, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvY : public SingleMthdTest {
	void emulate_mthd() override {
		uint16_t tv = nv03_pgraph_convert_xy(val);
		if (cls != 0x48 && extr(exp.debug[0], 14, 1) && exp.dvd_format & 1) {
			if (tv >= 0x8000 && tv < 0x8008)
				tv = 0x8000;
			else
				tv -= 8;
		}
		insrt(exp.d3d56_tlv_xy, 16, 16, tv);
		insrt(exp.valid[1], 4, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvZ : public SingleMthdTest {
	void emulate_mthd() override {
		exp.d3d56_tlv_z = val;
		insrt(exp.valid[1], 3, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvRhw : public SingleMthdTest {
	void emulate_mthd() override {
		exp.d3d56_tlv_rhw = val & ~0x3f;
		if (extr(exp.debug[2], 9, 1) && chipset.chipset >= 5) {
			if (extr(exp.d3d56_tlv_rhw, 0, 31) < 0xa800000)
				insrt(exp.d3d56_tlv_rhw, 0, 31, 0xa800000);
		}
		insrt(exp.valid[1], 2, 1, 1);
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		insrt(exp.valid[0], vidx, 1, 0);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdEmuD3D0TlvFogTri : public SingleMthdTest {
	void emulate_mthd() override {
		exp.d3d56_tlv_fog_tri_col1 = val ^ 0xff000000;
		insrt(exp.valid[1], 0, 7, 1);
	}
	using SingleMthdTest::SingleMthdTest;
};

class MthdD3D56TlvUv : public SingleMthdTest {
	int which_uv, which_vtx;
	bool fin;
	void adjust_orig_mthd() override {
		if (fin)
			insrt(orig.notify, 0, 1, 0);
		if (cls == 0x48) {
			// XXX: test me
			exp.valid[0] = 0;
			exp.valid[1] = 0;
			skip = true;
		}
	}
	void emulate_mthd() override {
		exp.d3d56_tlv_uv[which_vtx][which_uv] = val & ~0x3f;
		int vidx = (cls == 0x48 ? extr(exp.d3d56_tlv_fog_tri_col1, 0, 4) : idx);
		if (!which_uv || !which_vtx)
			insrt(exp.valid[1], 6 + which_vtx * 2 + which_uv, 1, 1);
		if (fin) {
			exp.vtx_xy[vidx][0] = extr(exp.d3d56_tlv_xy, 0, 16) | extr(exp.d3d56_tlv_color, 16, 16) << 16;
			exp.vtx_xy[vidx][1] = extr(exp.d3d56_tlv_xy, 16, 16) | extr(exp.d3d56_tlv_color, 0, 16) << 16;
			int xcstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][0], 0);
			int ycstat = nv04_pgraph_clip_status(&exp, exp.vtx_xy[vidx][1], 1);
			pgraph_set_xy_d(&exp, 0, vidx, vidx, false, false, false, xcstat);
			pgraph_set_xy_d(&exp, 1, vidx, vidx, false, false, false, ycstat);
			exp.vtx_u[vidx] = exp.d3d56_tlv_uv[0][0];
			exp.vtx_v[vidx] = exp.d3d56_tlv_uv[0][1];
			exp.vtx_m[vidx] = exp.d3d56_tlv_rhw;
			uint32_t msk = (cls == 0x55 ? 0x1ff : 0x7f);
			int ovidx = extr(exp.valid[0], 16, 4);
			if ((exp.valid[1] & msk) == msk) {
				insrt(exp.valid[0], vidx, 1, 1);
				if (cls != 0x48)
					insrt(exp.valid[0], 16, 4, vidx);
				exp.valid[1] &= ~msk;
			} else if (!extr(exp.valid[0], ovidx, 1)) {
				nv04_pgraph_blowup(&exp, 0x4000);
			}
			// sounds buggy...
			if (extr(exp.debug[3], 5, 1) && cls != 0x48)
				vidx = extr(exp.valid[0], 16, 4);
			if (cls == 0x55) {
				vidx &= 7;
				exp.vtx_u[vidx + 8] = exp.d3d56_tlv_uv[1][0];
				exp.vtx_v[vidx + 8] = exp.d3d56_tlv_uv[1][1];
				exp.vtx_xy[vidx + 8][0] = exp.d3d56_tlv_fog_tri_col1;
				exp.vtx_xy[vidx + 8][1] = exp.d3d56_tlv_z;
			} else {
				exp.vtx_xy[vidx + 16][0] = exp.d3d56_tlv_fog_tri_col1;
				exp.vtx_xy[vidx + 16][1] = exp.d3d56_tlv_z;
			}
			if (!extr(exp.surf_format, 8, 4) && extr(exp.debug[3], 22, 1))
				nv04_pgraph_blowup(&exp, 0x0200);
			if (cls == 0x48) {
				exp.misc24[1] = exp.d3d56_tlv_fog_tri_col1 & 0xffffff;
			}
		} else {
			insrt(exp.valid[0], vidx, 1, 0);
		}
	}
public:
	MthdD3D56TlvUv(hwtest::TestOptions &opt, uint32_t seed, const std::string &name, int trapbit, uint32_t cls, uint32_t mthd, uint32_t num, uint32_t stride, int which_vtx, int which_uv, bool fin)
	: SingleMthdTest(opt, seed, name, trapbit, cls, mthd, num, stride), which_uv(which_uv), which_vtx(which_vtx), fin(fin) {}
};

std::vector<SingleMthdTest *> EmuD3D0::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdPatch(opt, rnd(), "patch", -1, cls, 0x10c),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_tex", -1, cls, 0x184, 0, DMA_R | DMA_ALIGN),
		new MthdCtxClip(opt, rnd(), "ctx_clip", -1, cls, 0x188),
		new MthdCtxSurf(opt, rnd(), "ctx_color", -1, cls, 0x18c, 2),
		new MthdCtxSurf(opt, rnd(), "ctx_zeta", -1, cls, 0x190, 3),
		new MthdMissing(opt, rnd(), "missing", -1, cls, 0x200, 4),
		new MthdD3D56TexOffset(opt, rnd(), "tex_offset", -1, cls, 0x304, 3),
		new MthdEmuD3D0TexFormat(opt, rnd(), "tex_format", -1, cls, 0x308),
		new MthdEmuD3D0TexFilter(opt, rnd(), "tex_filter", -1, cls, 0x30c),
		new MthdD3D56FogColor(opt, rnd(), "fog_color", -1, cls, 0x310),
		new MthdEmuD3D0Config(opt, rnd(), "config", -1, cls, 0x314),
		new MthdEmuD3D0Alpha(opt, rnd(), "alpha", -1, cls, 0x318),
		new MthdEmuD3D0TlvFogTri(opt, rnd(), "tlv_fog_tri", -1, cls, 0x1000, 0x80, 0x20),
		new MthdD3D56TlvColor(opt, rnd(), "tlv_color", -1, cls, 0x1004, 0x80, 0x20),
		new MthdD3D56TlvX(opt, rnd(), "tlv_x", -1, cls, 0x1008, 0x80, 0x20),
		new MthdD3D56TlvY(opt, rnd(), "tlv_y", -1, cls, 0x100c, 0x80, 0x20),
		new MthdD3D56TlvZ(opt, rnd(), "tlv_z", -1, cls, 0x1010, 0x80, 0x20),
		new MthdD3D56TlvRhw(opt, rnd(), "tlv_rhw", -1, cls, 0x1014, 0x80, 0x20),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_u", -1, cls, 0x1018, 0x80, 0x20, 0, 0, false),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_v", -1, cls, 0x101c, 0x80, 0x20, 0, 1, true),
	};
}

std::vector<SingleMthdTest *> D3D5::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_a", -1, cls, 0x184, 0, DMA_R | DMA_ALIGN),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_b", -1, cls, 0x188, 1, DMA_R | DMA_ALIGN),
		new MthdCtxSurf3D(opt, rnd(), "ctx_surf3d", -1, cls, 0x18c, SURF2D_NV4),
		new MthdD3D56TexColorKey(opt, rnd(), "tex_color_key", -1, cls, 0x300),
		new MthdD3D56TexOffset(opt, rnd(), "tex_offset", -1, cls, 0x304, 3),
		new MthdD3D56TexFormat(opt, rnd(), "tex_format", -1, cls, 0x308, 3),
		new MthdD3D56TexFilter(opt, rnd(), "tex_filter", -1, cls, 0x30c, 3),
		new MthdD3D56Blend(opt, rnd(), "blend", -1, cls, 0x310),
		new MthdD3D56Config(opt, rnd(), "config", -1, cls, 0x314),
		new MthdD3D56FogColor(opt, rnd(), "fog_color", -1, cls, 0x318),
		new MthdD3D56TlvX(opt, rnd(), "tlv_x", -1, cls, 0x400, 0x10, 0x20),
		new MthdD3D56TlvY(opt, rnd(), "tlv_y", -1, cls, 0x404, 0x10, 0x20),
		new MthdD3D56TlvZ(opt, rnd(), "tlv_z", -1, cls, 0x408, 0x10, 0x20),
		new MthdD3D56TlvRhw(opt, rnd(), "tlv_rhw", -1, cls, 0x40c, 0x10, 0x20),
		new MthdD3D56TlvColor(opt, rnd(), "tlv_color", -1, cls, 0x410, 0x10, 0x20),
		new MthdD3D56TlvFogCol1(opt, rnd(), "tlv_fog_col1", -1, cls, 0x414, 0x10, 0x20),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_u", -1, cls, 0x418, 0x10, 0x20, 0, 0, false),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_v", -1, cls, 0x41c, 0x10, 0x20, 0, 1, true),
		new UntestedMthd(opt, rnd(), "draw", -1, cls, 0x600, 0x40),
	};
}

std::vector<SingleMthdTest *> D3D6::mthds() {
	return {
		new MthdNop(opt, rnd(), "nop", -1, cls, 0x100),
		new MthdNotify(opt, rnd(), "notify", -1, cls, 0x104),
		new MthdDmaNotify(opt, rnd(), "dma_notify", -1, cls, 0x180),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_a", -1, cls, 0x184, 0, DMA_R | DMA_ALIGN),
		new MthdDmaGrobj(opt, rnd(), "dma_tex_b", -1, cls, 0x188, 1, DMA_R | DMA_ALIGN),
		new MthdCtxSurf3D(opt, rnd(), "ctx_surf3d", -1, cls, 0x18c, SURF2D_NV4),
		new MthdD3D56TexOffset(opt, rnd(), "tex_0_offset", -1, cls, 0x308, 1),
		new MthdD3D56TexOffset(opt, rnd(), "tex_1_offset", -1, cls, 0x30c, 2),
		new MthdD3D56TexFormat(opt, rnd(), "tex_0_format", -1, cls, 0x310, 1),
		new MthdD3D56TexFormat(opt, rnd(), "tex_1_format", -1, cls, 0x314, 2),
		new MthdD3D56TexFilter(opt, rnd(), "tex_0_filter", -1, cls, 0x318, 1),
		new MthdD3D56TexFilter(opt, rnd(), "tex_1_filter", -1, cls, 0x31c, 2),
		new MthdD3D6CombineControl(opt, rnd(), "combine_0_control_alpha", -1, cls, 0x320, 0, 0),
		new MthdD3D6CombineControl(opt, rnd(), "combine_0_control_color", -1, cls, 0x324, 0, 1),
		new MthdD3D6CombineControl(opt, rnd(), "combine_1_control_alpha", -1, cls, 0x32c, 1, 0),
		new MthdD3D6CombineControl(opt, rnd(), "combine_1_control_color", -1, cls, 0x330, 1, 1),
		new MthdD3D6CombineFactor(opt, rnd(), "combine_factor", -1, cls, 0x334),
		new MthdD3D56Blend(opt, rnd(), "blend", -1, cls, 0x338),
		new MthdD3D56Config(opt, rnd(), "config", -1, cls, 0x33c),
		new MthdD3D6StencilFunc(opt, rnd(), "stencil_func", -1, cls, 0x340),
		new MthdD3D6StencilOp(opt, rnd(), "stencil_op", -1, cls, 0x344),
		new MthdD3D56FogColor(opt, rnd(), "fog_color", -1, cls, 0x348),
		new MthdD3D56TlvX(opt, rnd(), "tlv_x", -1, cls, 0x400, 8, 0x28),
		new MthdD3D56TlvY(opt, rnd(), "tlv_y", -1, cls, 0x404, 8, 0x28),
		new MthdD3D56TlvZ(opt, rnd(), "tlv_z", -1, cls, 0x408, 8, 0x28),
		new MthdD3D56TlvRhw(opt, rnd(), "tlv_rhw", -1, cls, 0x40c, 8, 0x28),
		new MthdD3D56TlvColor(opt, rnd(), "tlv_color", -1, cls, 0x410, 8, 0x28),
		new MthdD3D56TlvFogCol1(opt, rnd(), "tlv_fog_col1", -1, cls, 0x414, 8, 0x28),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_u_0", -1, cls, 0x418, 8, 0x28, 0, 0, false),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_v_0", -1, cls, 0x41c, 8, 0x28, 0, 1, false),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_u_1", -1, cls, 0x420, 8, 0x28, 1, 0, false),
		new MthdD3D56TlvUv(opt, rnd(), "tlv_v_1", -1, cls, 0x424, 8, 0x28, 1, 1, true),
		new UntestedMthd(opt, rnd(), "draw", -1, cls, 0x540, 0x30),
	};
}

}
}
