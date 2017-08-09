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
#include "nva.h"
#include <vector>
#include <memory>

namespace hwtest {
namespace pgraph {

#define AREG(t, ...) res.push_back(std::unique_ptr<Register>(new t(__VA_ARGS__)))

#define REG(a, m, n, f) AREG(SimpleMmioRegister, a, m, n, &pgraph_state::f)
#define REGF(a, m, n, f, fx) AREG(SimpleMmioRegister, a, m, n, &pgraph_state::f, fx)
#define IREGF(a, m, n, f, i, x, fx) AREG(IndexedMmioRegister<x>, a, m, n, &pgraph_state::f, i, fx)
#define IREG(a, m, n, f, i, x) IREGF(a, m, n, f, i, x, 0)

class Nv1DebugARegister : public SimpleMmioRegister {
public:
	Nv1DebugARegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "DEBUG_A", &pgraph_state::debug_a) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		if (extr(val, 0, 1))
			pgraph_reset(state);
	}
};

class Nv4DebugARegister : public SimpleMmioRegister {
public:
	Nv4DebugARegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "DEBUG_A", &pgraph_state::debug_a) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		if (val & 3) {
			state->xy_a &= 1 << 20;
			state->xy_misc_1[0] = 0;
			state->xy_misc_1[1] = 0;
			state->xy_misc_3 &= ~0x1100;
			state->xy_misc_4[0] &= ~0xff;
			state->xy_misc_4[1] &= ~0xff;
			state->xy_clip[0][0] = 0x55555555;
			state->xy_clip[0][1] = 0x55555555;
			state->xy_clip[1][0] = 0x55555555;
			state->xy_clip[1][1] = 0x55555555;
			state->valid[0] = 0;
			state->valid[1] = 0;
			state->misc24[0] = 0;
			state->misc24[1] = 0;
			state->misc24[2] = 0;
		}
		if (val & 0x11) {
			uint32_t offset_mask = pgraph_offset_mask(&state->chipset);
			for (int i = 0; i < 6; i++) {
				state->surf_offset[i] = 0;
				state->surf_base[i] = 0;
				state->surf_limit[i] = offset_mask | 0xf;
			}
			for (int i = 0; i < 5; i++)
				state->surf_pitch[i] = 0;
			for (int i = 0; i < 2; i++)
				state->surf_swizzle[i] = 0;
			state->surf_unk610 = 0;
			state->surf_unk614 = 0;
		}
		if (val & 0x101) {
			state->dma_eng_flags[0] &= ~0x1000;
			state->dma_eng_flags[1] &= ~0x1000;
		}
	}
};

class DebugBRegister : public SimpleMmioRegister {
public:
	DebugBRegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "DEBUG_B", &pgraph_state::debug_b) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		if (extr(val, 4, 1)) {
			insrt(state->xy_misc_1[0], 0, 1, 0);
		}
	}
};

class CelsiusDebugBRegister : public DebugBRegister {
public:
	using DebugBRegister::DebugBRegister;
	void write(int cnum, uint32_t val) override {
		// fuck you
		uint32_t mangled = val & 0x3fffffff;
		if (val & 1 << 30)
			mangled |= 1 << 31;
		if (val & 1 << 31)
			mangled |= 1 << 30;
		nva_wr32(cnum, addr, mangled);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_debug_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type == 1) {
		AREG(Nv1DebugARegister, 0x400080, 0x11111110);
		REG(0x400084, 0x31111101, "DEBUG_B", debug_b);
		REG(0x400088, 0x11111111, "DEBUG_C", debug_c);
	} else if (chipset.card_type == 3) {
		AREG(Nv1DebugARegister, 0x400080, 0x13311110);
		AREG(DebugBRegister, 0x400084, 0x10113301);
		REG(0x400088, 0x1133f111, "DEBUG_C", debug_c);
		REG(0x40008c, 0x1173ff31, "DEBUG_D", debug_d);
	} else if (chipset.card_type == 4) {
		bool is_nv5 = chipset.chipset >= 5;
		AREG(Nv4DebugARegister, 0x400080, 0x1337f000);
		AREG(DebugBRegister, 0x400084, (is_nv5 ? 0xf2ffb701 : 0x72113101));
		REG(0x400088, 0x11d7fff1, "DEBUG_C", debug_c);
		REG(0x40008c, (is_nv5 ? 0xfbffff73 : 0x11ffff33), "DEBUG_D", debug_d);
	} else if (chipset.card_type == 0x10) {
		bool is_nv11p = nv04_pgraph_is_nv11p(&chipset);
		bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
		bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
		// XXX DEBUG_A
		if (!is_nv11p) {
			AREG(CelsiusDebugBRegister, 0x400084, 0xfe11f701);
		} else {
			AREG(CelsiusDebugBRegister, 0x400084, 0xfe71f701);
		}
		REG(0x400088, 0xffffffff, "DEBUG_C", debug_c);
		if (!is_nv15p) {
			REG(0x40008c, 0xfffffc70, "DEBUG_D", debug_d);
		} else if (!is_nv17p) {
			REG(0x40008c, 0xffffff78, "DEBUG_D", debug_d);
		} else {
			REGF(0x40008c, 0xffffff78, "DEBUG_D", debug_d, 0x400);
		}
		if (!is_nv17p) {
			REG(0x400090, 0x00ffffff, "DEBUG_E", debug_e);
		} else {
			REG(0x400090, 0x1fffffff, "DEBUG_E", debug_e);
		}
	} else if (chipset.card_type == 0x20) {
		bool is_nv25p = nv04_pgraph_is_nv25p(&chipset);
		// XXX DEBUG_A
		AREG(DebugBRegister, 0x400084, 0x0011f7c1);
		if (!is_nv25p) {
			REG(0x40008c, 0xffffd77d, "DEBUG_D", debug_d);
			REG(0x400090, 0xfffff3ff, "DEBUG_E", debug_e);
			REG(0x400094, 0x000000ff, "DEBUG_F", debug_f);
		} else {
			REG(0x40008c, 0xffffdf7d, "DEBUG_D", debug_d);
			REG(0x400090, 0xffffffff, "DEBUG_E", debug_e);
		}
		REG(0x400098, 0xffffffff, "DEBUG_G", debug_g);
		REG(0x40009c, 0x00000fff, "DEBUG_FD_CHECK_SKIP", debug_fd_check_skip);
		if (is_nv25p) {
			REG(0x4000c0, 0x00000003, "DEBUG_L", debug_l);
		}
	} else if (chipset.card_type == 0x30) {
		// XXX DEBUG_A
		AREG(DebugBRegister, 0x400084, 0x7012f7c1);
		REG(0x40008c, 0xfffedf7d, "DEBUG_D", debug_d);
		REG(0x400090, 0x3fffffff, "DEBUG_E", debug_e);
		REG(0x400098, 0xffffffff, "DEBUG_G", debug_g);
		REG(0x40009c, 0xffffffff, "DEBUG_H", debug_h);
		REG(0x4000a0, 0xffffffff, "DEBUG_I", debug_i);
		REG(0x4000a4, 0x0000000f, "DEBUG_J", debug_j);
		REG(0x4000c0, 0x0000001e, "DEBUG_L", debug_l);
	} else {
		// XXX DEBUG_A
		AREG(DebugBRegister, 0x400084, 0x7010c7c1);
		REG(0x40008c, 0xe1fad155, "DEBUG_D", debug_d);
		REG(0x400090, 0x33ffffff, "DEBUG_E", debug_e);
		REG(0x4000a4, 0x0000000f, "DEBUG_J", debug_j);
		REG(0x4000b0, 0x0000003f, "DEBUG_K", debug_k);
		REG(0x4000c0, 0x00000006, "DEBUG_L", debug_l);
	}
	return res;
}

class CtxControlRegister : public SimpleMmioRegister {
public:
	CtxControlRegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "CTX_CONTROL", &pgraph_state::ctx_control) {}
	uint32_t read(int cnum) override {
		// XXX clean this up some day
		return nva_rd32(cnum, addr) & ~(1 << 20 | 1 << 8);
	}
};

class CtxSwitchARegister : public SimpleMmioRegister {
public:
	CtxSwitchARegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "CTX_SWITCH_A", &pgraph_state::ctx_switch_a) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		bool vre;
		if (state->chipset.card_type < 3) {
			vre = false;
		} else if (state->chipset.card_type < 0x10) {
			vre = extr(state->debug_c, 28, 1);
		} else {
			vre = extr(state->debug_d, 19, 1);
		}
		bool vr = extr(val, 31, 1) && vre;
		insrt(state->debug_b, 0, 1, vr);
		if (vr)
			pgraph_volatile_reset(state);
	}
};

class CtxSwitchCRegister : public SimpleMmioRegister {
public:
	CtxSwitchCRegister(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "CTX_SWITCH_C", &pgraph_state::ctx_switch_c) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		bool vre = extr(state->debug_d, 19, 1);
		bool vr = extr(val, 31, 1) && vre;
		insrt(state->debug_b, 0, 1, vr);
		if (vr)
			pgraph_volatile_reset(state);
	}
};

class CtxCacheRegister : public IndexedMmioRegister<8> {
public:
	CtxCacheRegister(uint32_t addr, uint32_t mask, std::string name, uint32_t (pgraph_state::*ptr)[8], int idx) :
		IndexedMmioRegister<8>(addr, mask, name, ptr, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		if (!state->fifo_enable)
			ref(state) = val & mask;
	}
};

std::vector<std::unique_ptr<Register>> pgraph_control_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 4) {
		REG(0x400140, 0x11111111, "INTR_EN", intr_en);
		REG(0x400144, 0x00011111, "INVALID_EN", invalid_en);
		if (chipset.card_type < 3) {
			AREG(CtxSwitchARegister, 0x400180, 0x807fffff);
		} else {
			AREG(CtxSwitchARegister, 0x400180, 0x3ff3f71f);
		}
		AREG(CtxControlRegister, 0x400190, 0x11010003);
		if (chipset.card_type >= 3) {
			REG(0x400194, 0x7f1fe000, "CTX_USER", ctx_user);
			for (int i = 0; i < 8; i++) {
				AREG(CtxCacheRegister, 0x4001a0 + i * 4, 0x3ff3f71f, "CTX_CACHE_A", &pgraph_state::ctx_cache_a, i);
			}
		}
		REG(0x400680, 0x0000ffff, "CTX_SWITCH_B", ctx_switch_b);
		if (chipset.card_type < 3) {
			REG(0x400684, 0x0011ffff, "NOTIFY", notify);
		} else {
			REG(0x400684, 0x00f1ffff, "NOTIFY", notify);
			REG(0x400688, 0x0000ffff, "CTX_SWITCH_I", ctx_switch_i);
			REG(0x40068c, 0x0001ffff, "CTX_SWITCH_C", ctx_switch_c);
		}
	} else {
		if (chipset.card_type < 0x10) {
			REG(0x400140, 0x00011311, "INTR_EN", intr_en);
		} else if (chipset.card_type < 0x20) {
			REG(0x400140, 0x01113711, "INTR_EN", intr_en);
		} else if (chipset.card_type < 0x40) {
			REG(0x400140, 0x011137d1, "INTR_EN", intr_en);
		} else {
			REG(0x40013c, 0x03111fd1, "INTR_EN", intr_en);
		}
		if (chipset.card_type < 0x10) {
			REG(0x400104, 0x00007800, "NSTATUS", nstatus);
		} else {
			REG(0x400104, 0x07800000, "NSTATUS", nstatus);
		}
		if (chipset.card_type < 0x10) {
			bool is_nv5 = chipset.chipset >= 5;
			uint32_t ctx_a_mask = is_nv5 ? 0x7f73f0ff : 0x0303f0ff;
			AREG(CtxSwitchARegister, 0x400160, ctx_a_mask);
			REG(0x400164, 0xffff3f03, "CTX_SWITCH_B", ctx_switch_b);
			REG(0x400168, 0xffffffff, "CTX_SWITCH_C", ctx_switch_c);
			REG(0x40016c, 0x0000ffff, "CTX_SWITCH_I", ctx_switch_i);

			AREG(CtxControlRegister, 0x400170, 0x11010003);
			REG(0x400174, 0x0f00e000, "CTX_USER", ctx_user);
			for (int i = 0; i < 8; i++) {
				AREG(CtxCacheRegister, 0x400180 + i * 4, ctx_a_mask, "CTX_CACHE_A", &pgraph_state::ctx_cache_a, i);
				AREG(CtxCacheRegister, 0x4001a0 + i * 4, 0xffff3f03, "CTX_CACHE_B", &pgraph_state::ctx_cache_b, i);
				AREG(CtxCacheRegister, 0x4001c0 + i * 4, 0xffffffff, "CTX_CACHE_C", &pgraph_state::ctx_cache_c, i);
				AREG(CtxCacheRegister, 0x4001e0 + i * 4, 0x0000ffff, "CTX_CACHE_I", &pgraph_state::ctx_cache_i, i);
			}
		} else if (chipset.card_type < 0x40) {
			AREG(CtxControlRegister, 0x400144, 0x11010003);
			if (!nv04_pgraph_is_nv15p(&chipset)) {
				REG(0x400148, 0x1f00e000, "CTX_USER", ctx_user);
			} else if (chipset.card_type < 0x20) {
				REG(0x400148, 0x9f00e000, "CTX_USER", ctx_user);
			} else {
				REG(0x400148, 0x9f00ff11, "CTX_USER", ctx_user);
			}
			uint32_t ctx_a_mask;
			if (!nv04_pgraph_is_nv11p(&chipset)) {
				ctx_a_mask = 0x7fb3f0ff;
			} else if (!nv04_pgraph_is_nv25p(&chipset)) {
				ctx_a_mask = 0x7ffff0ff;
			} else {
				ctx_a_mask = 0x7fffffff;
			}
			AREG(CtxSwitchARegister, 0x40014c, ctx_a_mask);
			REG(0x400150, 0xffff3f03, "CTX_SWITCH_B", ctx_switch_b);
			REG(0x400154, 0xffffffff, "CTX_SWITCH_C", ctx_switch_c);
			REG(0x400158, 0x0000ffff, "CTX_SWITCH_I", ctx_switch_i);
			REG(0x40015c, 0xffffffff, "CTX_SWITCH_T", ctx_switch_t);
			for (int i = 0; i < 8; i++) {
				uint32_t ctxc_a_mask;
				if (!nv04_pgraph_is_nv15p(&chipset)) {
					ctxc_a_mask = 0x7f33f0ff;
				} else if (!nv04_pgraph_is_nv11p(&chipset)) {
					ctxc_a_mask = 0x7fb3f0ff;
				} else if (!nv04_pgraph_is_nv25p(&chipset)) {
					ctxc_a_mask = 0x7ffff0ff;
				} else {
					ctxc_a_mask = 0x7fffffff;
				}
				AREG(CtxCacheRegister, 0x400160 + i * 4, ctxc_a_mask, "CTX_CACHE_A", &pgraph_state::ctx_cache_a, i);
				AREG(CtxCacheRegister, 0x400180 + i * 4, 0xffff3f03, "CTX_CACHE_B", &pgraph_state::ctx_cache_b, i);
				AREG(CtxCacheRegister, 0x4001a0 + i * 4, 0xffffffff, "CTX_CACHE_C", &pgraph_state::ctx_cache_c, i);
				AREG(CtxCacheRegister, 0x4001c0 + i * 4, 0x0000ffff, "CTX_CACHE_I", &pgraph_state::ctx_cache_i, i);
				AREG(CtxCacheRegister, 0x4001e0 + i * 4, 0xffffffff, "CTX_CACHE_T", &pgraph_state::ctx_cache_t, i);
			}
		} else {
			REG(0x400144, 0x8000e001, "CTX_USER", ctx_user);
			REG(0x400148, 0xffffffff, "CTX_SWITCH_A", ctx_switch_a);
			REG(0x40014c, 0xffffffff, "CTX_SWITCH_B", ctx_switch_b);
			AREG(CtxSwitchCRegister, 0x400150, 0x07ffffff);
			REG(0x400154, 0x00ffffff, "CTX_SWITCH_D", ctx_switch_d);
			REG(0x400158, 0x00ffffff, "CTX_SWITCH_I", ctx_switch_i);
			REG(0x40015c, 0xffffffff, "CTX_SWITCH_T", ctx_switch_t);
			for (int i = 0; i < 8; i++) {
				AREG(CtxCacheRegister, 0x400160 + i * 4, 0xffffffff, "CTX_CACHE_A", &pgraph_state::ctx_cache_a, i);
				AREG(CtxCacheRegister, 0x400180 + i * 4, 0xffffffff, "CTX_CACHE_B", &pgraph_state::ctx_cache_b, i);
				AREG(CtxCacheRegister, 0x4001a0 + i * 4, 0xffffffff, "CTX_CACHE_C", &pgraph_state::ctx_cache_c, i);
				AREG(CtxCacheRegister, 0x4001c0 + i * 4, 0xffffffff, "CTX_CACHE_D", &pgraph_state::ctx_cache_d, i);
				AREG(CtxCacheRegister, 0x4001e0 + i * 4, 0x00ffffff, "CTX_CACHE_I", &pgraph_state::ctx_cache_i, i);
				AREG(CtxCacheRegister, 0x400200 + i * 4, 0xffffffff, "CTX_CACHE_T", &pgraph_state::ctx_cache_t, i);
			}
			REG(0x400220, 0x00ffffff, "SRC2D_DMA", src2d_dma);
			REG(0x400224, 0x0000ffff, "SRC2D_PITCH", src2d_pitch);
			REG(0x400228, 0xffffffff, "SRC2D_OFFSET", src2d_offset);
		}
		if (chipset.card_type < 0x10) {
			REG(0x400714, 0x00110101, "NOTIFY", notify);
		} else {
			REG(0x400718, 0x73110101, "NOTIFY", notify);
		}
	}
	return res;
}

class BetaRegister : public SimpleMmioRegister {
public:
	BetaRegister(uint32_t addr) :
		SimpleMmioRegister(addr, 0x7f800000, "BETA", &pgraph_state::beta) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		if (val & 0x80000000)
			ref(state) = 0;
		else
			ref(state) = val & mask;
	}
	bool scan_test(int cnum, std::mt19937 &rnd) override {
		for (int i = 0; i < 1000; i++) {
			uint32_t orig = rnd();
			write(cnum, orig);
			uint32_t exp = orig & 0x7f800000;
			if (orig & 0x80000000)
				exp = 0;
			uint32_t real = read(cnum);
			if (real != exp) {
				printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
				return true;
			}
		}
		return false;
	}
};

class BitmapColor0Register : public IndexedMmioRegister<2> {
public:
	BitmapColor0Register(uint32_t addr) :
		IndexedMmioRegister<2>(addr, 0xffffffff, "BITMAP_COLOR", &pgraph_state::bitmap_color, 0) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val;
		insrt(state->valid[0], 17, 1, 1);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_rop_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 4) {
		for (int i = 0; i < 2; i++) {
			IREG(0x400600 + i * 8, 0x3fffffff, "PATTERN_MONO_RGB", pattern_mono_rgb, i, 2);
			IREG(0x400604 + i * 8, 0x000000ff, "PATTERN_MONO_A", pattern_mono_a, i, 2);
			IREG(0x400610 + i * 4, 0xffffffff, "PATTERN_MONO_BITMAP", pattern_mono_bitmap, i, 2);
		}
		REG(0x400618, 3, "PATTERN_CONFIG", pattern_config);
		IREG(0x40061c, 0x7fffffff, "BITMAP_COLOR", bitmap_color, 0, 2);
		if (chipset.card_type < 3)
			IREG(0x400620, 0x7fffffff, "BITMAP_COLOR", bitmap_color, 1, 2);
		REG(0x400624, 0xff, "ROP", rop);
		if (chipset.card_type < 3)
			REG(0x400628, 0x7fffffff, "PLANE", plane);
		REG(0x40062c, 0x7fffffff, "CHROMA", chroma);
		if (chipset.card_type < 3)
			AREG(BetaRegister, 0x400630);
		else
			AREG(BetaRegister, 0x400640);
	} else if (chipset.card_type < 0x20) {
		for (int i = 0; i < 2; i++) {
			IREG(0x400800 + i * 4, 0xffffffff, "PATTERN_MONO_COLOR", pattern_mono_color, i, 2);
			IREG(0x400808 + i * 4, 0xffffffff, "PATTERN_MONO_BITMAP", pattern_mono_bitmap, i, 2);
		}
		REG(0x400810, 0x13, "PATTERN_CONFIG", pattern_config);
		for (int i = 0; i < 64; i++) {
			IREG(0x400900 + i * 4, 0x00ffffff, "PATTERN_COLOR", pattern_color, i, 64);
		}
		AREG(BitmapColor0Register, 0x400600);
		REG(0x400604, 0xff, "ROP", rop);
		REG(0x400608, 0x7f800000, "BETA", beta);
		REG(0x40060c, 0xffffffff, "BETA4", beta4);
		REG(0x400814, 0xffffffff, "CHROMA", chroma);
		REG(0x400830, 0x3f3f3f3f, "CTX_FORMAT", ctx_format);
		if (nv04_pgraph_is_nv17p(&chipset)) {
			REG(0x400838, 0xffffffff, "UNK838", unk838);
			REG(0x40083c, 0xffffffff, "UNK83C", unk83c);
		}
	} else {
		for (int i = 0; i < 2; i++) {
			IREG(0x400b10 + i * 4, 0xffffffff, "PATTERN_MONO_COLOR", pattern_mono_color, i, 2);
			IREG(0x400b18 + i * 4, 0xffffffff, "PATTERN_MONO_BITMAP", pattern_mono_bitmap, i, 2);
		}
		REG(0x400b20, 0x13, "PATTERN_CONFIG", pattern_config);
		for (int i = 0; i < 64; i++) {
			IREG(0x400a00 + i * 4, 0x00ffffff, "PATTERN_COLOR", pattern_color, i, 64);
		}
		AREG(BitmapColor0Register, 0x400814);
		REG(0x400b00, 0xff, "ROP", rop);
		REG(0x400b04, 0x7f800000, "BETA", beta);
		REG(0x400b08, 0xffffffff, "BETA4", beta4);
		REG(0x40087c, 0xffffffff, "CHROMA", chroma);
		REG(0x400b0c, 0x3f3f3f3f, "CTX_FORMAT", ctx_format);
	}
	return res;
}

class SurfOffsetRegister : public IndexedMmioRegister<9> {
public:
	SurfOffsetRegister(uint32_t addr, int idx, uint32_t mask) :
		IndexedMmioRegister<9>(addr, mask, "SURF_OFFSET", &pgraph_state::surf_offset, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 3, 1, 1);
		if (state->chipset.card_type >= 0x20 && !extr(state->surf_unk880, 20, 1)) {
			insrt(state->surf_unk888, idx & ~1, 1, 0);
			insrt(state->surf_unk888, idx | 1, 1, 0);
			if (nv04_pgraph_is_nv25p(&state->chipset)) {
				insrt(state->surf_unk89c, (idx & ~1) * 4, 4, 0);
				insrt(state->surf_unk89c, (idx | 1) * 4, 4, 0);
			}
		}
	}
};

class SurfBaseRegister : public IndexedMmioRegister<9> {
public:
	SurfBaseRegister(uint32_t addr, int idx, uint32_t mask) :
		IndexedMmioRegister<9>(addr, mask, "SURF_BASE", &pgraph_state::surf_base, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		if (state->chipset.card_type >= 0x20 && !extr(state->surf_unk880, 20, 1)) {
			insrt(state->surf_unk888, idx & ~1, 1, 0);
			insrt(state->surf_unk888, idx | 1, 1, 0);
			if (nv04_pgraph_is_nv25p(&state->chipset)) {
				insrt(state->surf_unk89c, (idx & ~1) * 4, 4, 0);
				insrt(state->surf_unk89c, (idx | 1) * 4, 4, 0);
			}
		}
	}
};

class SurfPitchRegister : public IndexedMmioRegister<9> {
public:
	SurfPitchRegister(uint32_t addr, int idx, uint32_t mask) :
		IndexedMmioRegister<9>(addr, mask, "SURF_PITCH", &pgraph_state::surf_pitch, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 2, 1, 1);
	}
};

class SurfUnk800Nv34Register : public SimpleMmioRegister {
public:
	SurfUnk800Nv34Register(uint32_t addr, uint32_t mask) :
		SimpleMmioRegister(addr, mask, "SURF_UNK800", &pgraph_state::surf_unk800) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->surf_unk800, 8, 1, extr(state->surf_unk800, 0, 1));
		insrt(state->surf_unk800, 12, 1, extr(state->surf_unk800, 4, 1));
	}
};

std::vector<std::unique_ptr<Register>> pgraph_canvas_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 4) {
		uint32_t canvas_mask;
		if (chipset.card_type < 3) {
			canvas_mask = 0x0fff0fff;
			REG(0x400634, 0x01111011, "CANVAS_CONFIG", canvas_config);
			REG(0x400688, 0xffffffff, "CANVAS_MIN", dst_canvas_min);
			REG(0x40068c, canvas_mask, "CANVAS_MAX", dst_canvas_max);
		} else {
			canvas_mask = chipset.is_nv03t ? 0x7fff07ff : 0x3fff07ff;
			REG(0x400550, canvas_mask, "SRC_CANVAS_MIN", src_canvas_min);
			REG(0x400554, canvas_mask, "SRC_CANVAS_MAX", src_canvas_max);
			REG(0x400558, canvas_mask, "DST_CANVAS_MIN", dst_canvas_min);
			REG(0x40055c, canvas_mask, "DST_CANVAS_MAX", dst_canvas_max);
			uint32_t offset_mask = chipset.is_nv03t ? 0x007fffff : 0x003fffff;
			for (int i = 0; i < 4; i++) {
				IREG(0x400630 + i * 4, offset_mask & ~0xf, "SURF_OFFSET", surf_offset, i, 9);
				IREG(0x400650 + i * 4, 0x1ff0, "SURF_PITCH", surf_pitch, i, 9);
			}
			REG(0x4006a8, 0x7777, "SURF_FORMAT", surf_format);
		}
		for (int i = 0; i < 2; i++) {
			IREG(0x400690 + i * 8, canvas_mask, "CLIPRECT_MIN", cliprect_min, i, 2);
			IREG(0x400694 + i * 8, canvas_mask, "CLIPRECT_MAX", cliprect_max, i, 2);
		}
		REG(0x4006a0, 0x113, "CLIPRECT_CTRL", cliprect_ctrl);
	} else if (chipset.card_type < 0x20) {
		uint32_t offset_mask = pgraph_offset_mask(&chipset);
		uint32_t pitch_mask = pgraph_pitch_mask(&chipset);
		for (int i = 0; i < 6; i++) {
			AREG(SurfOffsetRegister, 0x400640 + i * 4, i, offset_mask);
			IREG(0x400658 + i * 4, offset_mask, "SURF_BASE", surf_base, i, 9);
			IREGF(0x400684 + i * 4, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, i, 9, 0xf);
		}
		for (int i = 0; i < 5; i++) {
			AREG(SurfPitchRegister, 0x400670 + i * 4, i, pitch_mask);
		}
		for (int i = 0; i < 2; i++) {
			IREG(0x40069c + i * 4, 0x0f0f0000, "SURF_SWIZZLE", surf_swizzle, i, 2);
		}
		uint32_t st_mask;
		if (!nv04_pgraph_is_nv15p(&chipset))
			st_mask = 3;
		else if (!nv04_pgraph_is_nv11p(&chipset))
			st_mask = 0x77777703;
		else if (!nv04_pgraph_is_nv17p(&chipset))
			st_mask = 0x77777713;
		else
			st_mask = 0xf77777ff;
		REG(chipset.card_type >= 0x10 ? 0x400710 : 0x40070c, st_mask, "SURF_TYPE", surf_type);
		REG(0x400724, 0xffffff, "SURF_FORMAT", surf_format);
		REG(chipset.card_type >= 0x10 ? 0x400714 : 0x400710,
			nv04_pgraph_is_nv17p(&chipset) ? 0x3f731f3f : 0x0f731f3f,
			"CTX_VALID", ctx_valid);
		REG(0x400610, (chipset.card_type < 4 ? 0xe0000000 : 0xf8000000) | offset_mask, "SURF_UNK610", surf_unk610);
		REG(0x400614, 0xc0000000 | offset_mask, "SURF_UNK614", surf_unk614);
		if (nv04_pgraph_is_nv17p(&chipset)) {
			REG(0x4006b0, 0xffffffff, "UNK6B0", unk6b0);
		}
	} else {
		uint32_t offset_mask = pgraph_offset_mask(&chipset);
		uint32_t base_mask = offset_mask;
		uint32_t limit_fixed = 0x3f;
		if (chipset.chipset == 0x34) {
			base_mask |= 0xc0000000;
			limit_fixed = 0xf;
		}
		uint32_t pitch_mask = pgraph_pitch_mask(&chipset);
		for (int i = 0; i < 6; i++) {
			if (chipset.card_type == 0x40 && i == 1)
				continue;
			AREG(SurfOffsetRegister, 0x400820 + i * 4, i, offset_mask);
			AREG(SurfBaseRegister, 0x400838 + i * 4, i, base_mask);
			IREGF(0x400864 + i * 4, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, i, 9, limit_fixed);
		}
		for (int i = 0; i < 5; i++) {
			if (chipset.card_type == 0x40 && i == 1)
				continue;
			AREG(SurfPitchRegister, 0x400850 + i * 4, i, pitch_mask);
		}
		for (int i = 0; i < 2; i++) {
			IREG(0x400818 + i * 4, 0x0f0f0000, "SURF_SWIZZLE", surf_swizzle, i, 2);
		}
		if (nv04_pgraph_is_nv25p(&chipset)) {
			AREG(SurfOffsetRegister, 0x400880, 6, offset_mask);
			AREG(SurfBaseRegister, 0x400884, 6, base_mask);
			AREG(SurfPitchRegister, 0x400888, 6, pitch_mask);
			IREGF(0x40088c, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, 6, 9, limit_fixed);
		}
		uint32_t st_mask;
		if (!nv04_pgraph_is_nv25p(&chipset))
			st_mask = 0x77777733;
		else
			st_mask = 0x77777773;
		REG(0x400710, st_mask, "SURF_TYPE", surf_type);
		if (chipset.card_type < 0x30)
			REG(0x400724, 0xffffff, "SURF_FORMAT", surf_format);
		else
			REG(0x400724, 0x3fffffff, "SURF_FORMAT", surf_format);
		uint32_t ctx_valid_mask;
		if (!nv04_pgraph_is_nv25p(&chipset))
			ctx_valid_mask = 0x0f731f3f;
		else if (chipset.card_type < 0x40)
			ctx_valid_mask = 0x0f733f7f;
		else
			ctx_valid_mask = 0x0f77ffff;
		REG(0x400714, ctx_valid_mask, "CTX_VALID", ctx_valid);
		if (chipset.chipset != 0x34) {
			if (chipset.card_type < 0x30)
				REG(0x400800, 0xfff31f1f, "SURF_UNK800", surf_unk800);
			else
				REG(0x400800, 0xf1ffdf1f, "SURF_UNK800", surf_unk800);
			if (chipset.card_type < 0x40) {
				REG(0x40080c, 0xfffffffc, "SURF_UNK80C", surf_unk80c);
				REG(0x400810, 0xfffffffc, "SURF_UNK810", surf_unk810);
				if (!nv04_pgraph_is_nv25p(&chipset)) {
					REG(0x400880, 0xffffffff, "SURF_UNK880", surf_unk880);
					REG(0x400888, 0x0000003f, "SURF_UNK888", surf_unk888);
				} else {
					REG(0x400890, 0xffffffff, "SURF_UNK880", surf_unk880);
					REG(0x400898, 0x0000007f, "SURF_UNK888", surf_unk888);
					REG(0x40089c, 0x0fffffff, "SURF_UNK89C", surf_unk89c);
				}
			} else {
				REG(0x40080c, 0xffffffff, "SURF_UNK80C", surf_unk80c);
				REG(0x400810, 0xffffffff, "SURF_UNK810", surf_unk810);
				REG(0x4009b0, 0xffffffff, "SURF_UNK9B0", surf_unk9b0);
				REG(0x4009b4, 0xffffffff, "SURF_UNK9B4", surf_unk9b4);
				REG(0x4009b8, 0xffffffff, "SURF_UNK9B8", surf_unk9b8);
				REG(0x4009bc, 0xffffffff, "SURF_UNK9BC", surf_unk9bc);
				AREG(SurfOffsetRegister, 0x4009c0, 7, offset_mask);
				AREG(SurfBaseRegister, 0x4009c4, 7, base_mask);
				AREG(SurfPitchRegister, 0x4009c8, 7, pitch_mask);
				IREGF(0x4009cc, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, 7, 9, limit_fixed);
				AREG(SurfOffsetRegister, 0x4009d0, 8, offset_mask);
				AREG(SurfBaseRegister, 0x4009d4, 8, base_mask);
				AREG(SurfPitchRegister, 0x4009d8, 8, pitch_mask);
				IREGF(0x4009dc, 1 << 31 | offset_mask, "SURF_LIMIT", surf_limit, 8, 9, limit_fixed);
			}
		} else {
			AREG(SurfUnk800Nv34Register, 0x400800, 0x00001f17);
			REG(0x40080c, 0xfffffff0, "SURF_UNK80C", surf_unk80c);
			REG(0x400810, 0xfffffff0, "SURF_UNK810", surf_unk810);
			REG(0x400890, 0xffffffff, "SURF_UNK880", surf_unk880);
			REG(0x4008a4, 0xffffffff, "SURF_UNK8A4", surf_unk8a4);
			REG(0x4008a8, 0xffffffff, "SURF_UNK8A8", surf_unk8a8);
		}
	}
	return res;
}

class XyClipRegister : public MmioRegister {
public:
	int xy;
	int idx;
	XyClipRegister(int xy, int idx) :
		MmioRegister(0x400524 + xy * 8 + idx * 4, 0xffffffff), xy(xy), idx(idx) {}
	std::string name() override {
		return "XY_CLIP[" + std::to_string(xy) + "][" + std::to_string(idx) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return state->xy_clip[xy][idx]; }
};

class Misc32_0Register : public IndexedMmioRegister<4> {
public:
	Misc32_0Register() :
		IndexedMmioRegister<4>(0x40050c, 0xffffffff, "MISC32", &pgraph_state::misc32, 0) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 16, 1, 1);
	}
};

class XyARegister : public SimpleMmioRegister {
public:
	XyARegister(uint32_t addr, uint32_t mask, uint32_t fixed) :
		SimpleMmioRegister(addr, mask, "XY_A", &pgraph_state::xy_a, fixed) {}
	void gen(struct pgraph_state *state, int cnum, std::mt19937 &rnd) override {
		sim_write(state, rnd());
	}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		if (!extr(val, 1, 17))
			insrt(val, 18, 1, 1);
		ref(state) = val & (mask | fixed);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_vstate_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type < 3) {
		REG(0x400640, 0xf1ff11ff, "XY_A", xy_a);
		IREG(0x400644, 0x03177331, "XY_B", xy_misc_1, 0, 2);
		IREG(0x400648, 0x30ffffff, "XY_D", xy_misc_4, 0, 2);
		IREG(0x40064c, 0x30ffffff, "XY_D", xy_misc_4, 1, 2);
		IREG(0x400650, 0x111ff1ff, "VALID", valid, 0, 2);
		IREG(0x400654, 0xffffffff, "MISC32", misc32, 0, 4);
		REG(0x400658, 0xffff00ff, "SUBDIVIDE", subdivide);
		REG(0x40065c, 0xffff0113, "XY_E", edgefill);
	} else {
		uint32_t xy_a_mask = 0xf013ffff;
		if (chipset.card_type >= 0x10)
			xy_a_mask |= 0x01000000;
		uint32_t xy_a_fixed = (chipset.chipset == 0x34 || chipset.card_type >= 0x40) ? 0x40000 : 0;
		uint32_t xy_b_mask = chipset.card_type >= 4 ? 0x00111031 : 0x0f177331;
		AREG(XyARegister, 0x400514, xy_a_mask, xy_a_fixed);
		IREG(0x400518, xy_b_mask, "XY_B", xy_misc_1, 0, 2);
		IREG(0x40051c, xy_b_mask, "XY_B", xy_misc_1, 1, 2);
		REG(0x400520, 0x7f7f1111, "XY_C", xy_misc_3);
		IREG(0x400500, 0x300000ff, "XY_D", xy_misc_4, 0, 2);
		IREG(0x400504, 0x300000ff, "XY_D", xy_misc_4, 1, 2);
		AREG(XyClipRegister, 0, 0);
		AREG(XyClipRegister, 0, 1);
		AREG(XyClipRegister, 1, 0);
		AREG(XyClipRegister, 1, 1);
		IREG(0x400510, 0x00ffffff, "MISC24", misc24, 0, 3);
		IREG(0x400570, 0x00ffffff, "MISC24", misc24, 1, 3);
		if (chipset.card_type >= 4)
			IREG(0x400574, 0x00ffffff, "MISC24", misc24, 2, 3);
		res.push_back(std::unique_ptr<Register>(new Misc32_0Register()));
		if (chipset.card_type < 4) {
			IREG(0x40054c, 0xffffffff, "MISC32", misc32, 1, 4);
		} else {
			IREG(0x40057c, 0xffffffff, "MISC32", misc32, 1, 4);
			IREG(0x400580, 0xffffffff, "MISC32", misc32, 2, 4);
			IREG(0x400584, 0xffffffff, "MISC32", misc32, 3, 4);
			if (chipset.card_type < 0x10) {
				REG(0x400760, 0xffffffff, "DMA_PITCH", dma_pitch);
				REG(0x400764, 0x0000033f, "DVD_FORMAT", dvd_format);
				REG(0x400768, 0x01030000, "SIFM_MODE", sifm_mode);
			} else {
				REG(0x400770, 0xffffffff, "DMA_PITCH", dma_pitch);
				REG(0x400774, 0x0000033f, "DVD_FORMAT", dvd_format);
				REG(0x400778, 0x01030000, "SIFM_MODE", sifm_mode);
				REG(0x400588, 0x0000ffff, "TFC_UNK588", unk588);
				REG(0x40058c, 0x0001ffff, "TFC_UNK58C", unk58c);
			}
		}
		if (chipset.card_type < 4) {
			IREG(0x400508, 0xffffffff, "VALID", valid, 0, 2);
		} else {
			uint32_t v1_mask = chipset.card_type < 0x10 ? 0x1fffffff : 0xdfffffff;
			IREG(0x400508, 0xf07fffff, "VALID", valid, 0, 2);
			if (chipset.card_type < 0x20)
				IREG(0x400578, v1_mask, "VALID", valid, 1, 2);
		}
	}
	return res;
}

class D3D0ConfigRegister : public SimpleMmioRegister {
public:
	D3D0ConfigRegister() :
		SimpleMmioRegister(0x400644, 0xf77fbdf3, "D3D0_CONFIG", &pgraph_state::d3d0_config) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[0], 26, 1, 1);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_d3d0_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	REG(0x4005c0, 0xffffffff, "D3D0_TLV_XY", d3d0_tlv_xy);
	REG(0x4005c4, 0xffffffff, "D3D0_TLV_UV", d3d0_tlv_uv);
	REG(0x4005c8, 0x0000ffff, "D3D0_TLV_Z", d3d0_tlv_z);
	REG(0x4005cc, 0xffffffff, "D3D0_TLV_COLOR", d3d0_tlv_color);
	REG(0x4005d0, 0xffffffff, "D3D0_TLV_FOG_TRI", d3d0_tlv_fog_tri_col1);
	REG(0x4005d4, 0xffffffff, "D3D0_TLV_RHW", d3d0_tlv_rhw);
	AREG(D3D0ConfigRegister);
	REG(0x4006c8, 0x00000fff, "D3D0_ALPHA", d3d0_alpha);
	for (int i = 0; i < 16; i++) {
		IREG(0x400580 + i * 4, 0xffffff, "VTX_Z", vtx_z, i, 16);
	}
	return res;
}

class D3D56RcAlphaRegister : public IndexedMmioRegister<2> {
public:
	D3D56RcAlphaRegister(int idx) :
		IndexedMmioRegister<2>(0x400590 + idx * 8, 0xfd1d1d1d, "D3D56_RC_ALPHA", &pgraph_state::d3d56_rc_alpha, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 28 - idx * 2, 1, 1);
	}
};

class D3D56RcColorRegister : public IndexedMmioRegister<2> {
public:
	D3D56RcColorRegister(int idx) :
		IndexedMmioRegister<2>(0x400594 + idx * 8, 0xff1f1f1f, "D3D56_RC_COLOR", &pgraph_state::d3d56_rc_color, idx) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 27 - idx * 2, 1, 1);
	}
};

class D3D56TlvUvRegister : public MmioRegister {
public:
	int idx;
	int uv;
	D3D56TlvUvRegister(int idx, int uv) :
		MmioRegister(0x4005c4 + idx * 8 + uv * 4, 0xffffffc0), idx(idx), uv(uv) {}
	std::string name() override {
		return "D3D56_TLV_UV[" + std::to_string(idx) + "][" + std::to_string(uv) + "]";
	}
	uint32_t &ref(struct pgraph_state *state) override { return state->d3d56_tlv_uv[idx][uv]; }
};

class D3D56ConfigRegister : public SimpleMmioRegister {
public:
	D3D56ConfigRegister() :
		SimpleMmioRegister(0x400818, 0xffff5fff, "D3D56_CONFIG", &pgraph_state::d3d56_config) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 17, 1, 1);
	}
};

class D3D56StencilFuncRegister : public SimpleMmioRegister {
public:
	D3D56StencilFuncRegister() :
		SimpleMmioRegister(0x40081c, 0xfffffff1, "D3D56_STENCIL_FUNC", &pgraph_state::d3d56_stencil_func) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 18, 1, 1);
	}
};

class D3D56StencilOpRegister : public SimpleMmioRegister {
public:
	D3D56StencilOpRegister() :
		SimpleMmioRegister(0x400820, 0x00000fff, "D3D56_STENCIL_OP", &pgraph_state::d3d56_stencil_op) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 19, 1, 1);
	}
};

class D3D56BlendRegister : public SimpleMmioRegister {
public:
	D3D56BlendRegister() :
		SimpleMmioRegister(0x400824, 0xff1111ff, "D3D56_BLEND", &pgraph_state::d3d56_blend) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		ref(state) = val & mask;
		insrt(state->valid[1], 20, 1, 1);
	}
};

std::vector<std::unique_ptr<Register>> pgraph_d3d56_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	for (int i = 0; i < 2; i++) {
		AREG(D3D56RcAlphaRegister, i);
		AREG(D3D56RcColorRegister, i);
	}
	for (int i = 0; i < 2; i++) {
		IREG(0x4005a8 + i * 4, 0xfffff7a6, "D3D56_TEX_FORMAT", d3d56_tex_format, i, 2);
		IREG(0x4005b0 + i * 4, 0xffff9e1e, "D3D56_TEX_FILTER", d3d56_tex_filter, i, 2);
	}
	REG(0x4005c0, 0xffffffff, "D3D56_TLV_XY", d3d56_tlv_xy);
	AREG(D3D56TlvUvRegister, 0, 0);
	AREG(D3D56TlvUvRegister, 0, 1);
	AREG(D3D56TlvUvRegister, 1, 0);
	AREG(D3D56TlvUvRegister, 1, 1);
	REG(0x4005d4, 0xffffffff, "D3D56_TLV_Z", d3d56_tlv_z);
	REG(0x4005d8, 0xffffffff, "D3D56_TLV_COLOR", d3d56_tlv_color);
	REG(0x4005dc, 0xffffffff, "D3D56_TLV_FOG_TRI_COL1", d3d56_tlv_fog_tri_col1);
	REG(0x4005e0, 0xffffffc0, "D3D56_TLV_RHW", d3d56_tlv_rhw);
	AREG(D3D56ConfigRegister);
	AREG(D3D56StencilFuncRegister);
	AREG(D3D56StencilOpRegister);
	AREG(D3D56BlendRegister);
	for (int i = 0; i < 16; i++) {
		IREG(0x400d00 + i * 4, 0xffffffc0, "VTX_U", vtx_u, i, 16);
		IREG(0x400d40 + i * 4, 0xffffffc0, "VTX_V", vtx_v, i, 16);
		IREG(0x400d80 + i * 4, 0xffffffc0, "VTX_RHW", vtx_m, i, 16);
	}
	return res;
}

class CelsiusXfMiscBRegister : public SimpleMmioRegister {
public:
	CelsiusXfMiscBRegister() :
		SimpleMmioRegister(0x400f44, 0xffffffff, "CELSIUS_XF_MISC_B", &pgraph_state::celsius_xf_misc_b) {}
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		state->celsius_pipe_junk[2] = state->celsius_xf_misc_a;
		state->celsius_pipe_junk[3] = state->celsius_xf_misc_b;
	}
};

std::vector<std::unique_ptr<Register>> pgraph_fe3d_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	if (chipset.card_type == 0x10) {
		for (int i = 0; i < 8; i++) {
			IREG(0x400f00 + i * 4, 0x0fff0fff, "CELSIUS_CLIP_RECT_HORIZ", celsius_clip_rect_horiz, i, 8);
			IREG(0x400f20 + i * 4, 0x0fff0fff, "CELSIUS_CLIP_RECT_VERT", celsius_clip_rect_vert, i, 8);
		}
		REG(0x400f40, 0x3bffffff, "CELSIUS_XF_MISC_A", celsius_xf_misc_a);
		AREG(CelsiusXfMiscBRegister);
		REG(0x400f48, 0x17ff0117, "CELSIUS_CONFIG_C", celsius_config_c);
		REG(0x400f4c, 0xffffffff, "CELSIUS_DMA", celsius_dma);
	} else if (chipset.card_type == 0x20) {
		REG(0x400f5c, 0x01ffbffd, "FE3D_MISC", fe3d_misc);
		REG(0x400f60, 0xf3fff3ff, "FE3D_STATE_CURVE", fe3d_state_curve);
		REG(0x400f64, 0x07ffffff, "FE3D_STATE_SWATCH", fe3d_state_swatch);
		REG(0x400f68, 0xff000077, "FE3D_STATE_TRANSITION", fe3d_state_transition);
		for (int i = 0; i < 3; i++) {
			IREG(0x400f6c + i * 4, 0xffffffff, "FE3D_EMU_MATERIAL_FACTOR_RGB", fe3d_emu_material_factor_rgb, i, 3);
		}
		for (int i = 0; i < 3; i++) {
			IREG(0x400f78 + i * 4, 0xffffffff, "FE3D_EMU_LIGHT_MODEL_AMBIENT", fe3d_emu_light_model_ambient, i, 3);
		}
		REG(0x400f84, 0x0000ffff, "FE3D_DMA_STATE", fe3d_dma_state);
		REG(0x400f90, 0xffffffff, "FE3D_SHADOW_BEGIN_PATCH_A", fe3d_shadow_begin_patch_a);
		REG(0x400f94, 0xffffffff, "FE3D_SHADOW_BEGIN_PATCH_B", fe3d_shadow_begin_patch_b);
		REG(0x400f98, 0x7fffffff, "FE3D_SHADOW_BEGIN_PATCH_C", fe3d_shadow_begin_patch_c);
		REG(0x400f9c, 0x0001c03f, "FE3D_SHADOW_BEGIN_PATCH_D", fe3d_shadow_begin_patch_d);
		REG(0x400fa0, 0x00000007, "FE3D_SHADOW_CURVE", fe3d_shadow_curve);
		REG(0x400fa4, 0xffffffff, "FE3D_SHADOW_BEGIN_TRANSITION_A", fe3d_shadow_begin_transition_a);
		REG(0x400fa8, 0xffffffff, "FE3D_SHADOW_BEGIN_TRANSITION_B", fe3d_shadow_begin_transition_b);
		REG(0x400fb4, 0xfffcffff, "XF_MODE_B", xf_mode_b);
		REG(0x400fb8, 0xffffffff, "XF_MODE_A", xf_mode_a);
		for (int i = 0; i < 2; i++) {
			IREG(0x400fbc + i * 4, 0xfff7fff7, "XF_MODE_T", xf_mode_t, i ^ 1, 4);
		}
		REG(0x400fc4, 0x0000ffff, "FE3D_XF_LOAD_POS", fe3d_xf_load_pos);
	} else if (chipset.card_type >= 0x30) {
		if (chipset.card_type == 0x30) {
			REG(0x400f5c, 0xff1ffff1, "FE3D_MISC", fe3d_misc);
		} else {
			REG(0x400f5c, 0xfffffff3, "FE3D_MISC", fe3d_misc);
		}
		REG(0x400f60, 0xffffffff, "FE3D_SHADOW_CLIP_RECT_HORIZ", fe3d_shadow_clip_rect_horiz);
		REG(0x400f64, 0xffffffff, "FE3D_SHADOW_CLIP_RECT_VERT", fe3d_shadow_clip_rect_vert);
		if (chipset.card_type == 0x30) {
			REG(0x400f84, 0x0000ffff, "FE3D_DMA_STATE", fe3d_dma_state);
			REG(0x400fb8, 0x0000003f, "XF_MODE_C", xf_mode_c);
			REG(0x400fbc, 0xfedfffff, "XF_MODE_B", xf_mode_b);
			REG(0x400fc0, 0xffffffff, "XF_MODE_A", xf_mode_a);
			for (int i = 0; i < 4; i++) {
				IREG(0x400fc4 + i * 4, 0xfff7fff7, "XF_MODE_T", xf_mode_t, i ^ 3, 4);
			}
			REG(0x400fd4, 0x01ff01ff, "FE3D_XF_LOAD_POS", fe3d_xf_load_pos);
		} else {
			REG(0x400f84, 0x00ffffff, "FE3D_DMA_STATE", fe3d_dma_state);
		}
	}
	return res;
}

class CelsiusRegister : public MmioRegister {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		pgraph_celsius_icmd(state, extr(addr, 2, 6), ref(state), true);
	}
	using MmioRegister::MmioRegister;
};

class SimpleCelsiusRegister : public SimpleMmioRegister {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		pgraph_celsius_icmd(state, extr(addr, 2, 6), ref(state), true);
	}
	using SimpleMmioRegister::SimpleMmioRegister;
};

template<int n>
class IndexedCelsiusRegister : public IndexedMmioRegister<n> {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		uint32_t rval = IndexedMmioRegister<n>::ref(state);
		uint32_t ad = IndexedMmioRegister<n>::addr;
		pgraph_celsius_icmd(state, extr(ad, 2, 6), rval, true);
	}
	using IndexedMmioRegister<n>::IndexedMmioRegister;
};

#define CREG(a, m, n, f) res.push_back(std::unique_ptr<Register>(new SimpleCelsiusRegister(a, m, n, &pgraph_state::f)))
#define ICREGF(a, m, n, f, i, x, fx) res.push_back(std::unique_ptr<Register>(new IndexedCelsiusRegister<x>(a, m, n, &pgraph_state::f, i, fx)))
#define ICREG(a, m, n, f, i, x) ICREGF(a, m, n, f, i, x, 0)

std::vector<std::unique_ptr<Register>> pgraph_celsius_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	bool is_nv15p = nv04_pgraph_is_nv15p(&chipset);
	bool is_nv17p = nv04_pgraph_is_nv17p(&chipset);
	for (int i = 0; i < 2; i++) {
		ICREG(0x400e00 + i * 4, 0xffffffff, "BUNDLE_TEX_OFFSET", bundle_tex_offset, i, 0x10);
		ICREG(0x400e08 + i * 4, 0xffffffc1, "BUNDLE_TEX_PALETTE", bundle_tex_palette, i, 0x10);
		ICREG(0x400e10 + i * 4, is_nv17p ? 0xffffffde : 0xffffffd6, "BUNDLE_TEX_FORMAT", bundle_tex_format, i, 0x10);
		ICREG(0x400e18 + i * 4, 0x7fffffff, "BUNDLE_TEX_CONTROL_A", bundle_tex_control_a, i, 0x10);
		ICREG(0x400e20 + i * 4, 0xffff0000, "BUNDLE_TEX_CONTROL_B", bundle_tex_control_b, i, 0x10);
		ICREG(0x400e28 + i * 4, 0xffffffff, "BUNDLE_TEX_CONTROL_C", bundle_tex_control_c, i, 2);
		ICREG(0x400e30 + i * 4, 0x07ff07ff, "BUNDLE_TEX_RECT", bundle_tex_rect, i, 0x10);
		ICREG(0x400e38 + i * 4, 0x77001fff, "BUNDLE_TEX_FILTER", bundle_tex_filter, i, 0x10);
	}
	for (int i = 0; i < 2; i++) {
		ICREG(0x400e40 + i * 4, 0xffffffff, "BUNDLE_RC_IN_ALPHA", bundle_rc_in_alpha, i, 8);
		ICREG(0x400e48 + i * 4, 0xffffffff, "BUNDLE_RC_IN_COLOR", bundle_rc_in_color, i, 8);
		if (i == 0) {
			ICREG(0x400e50 + i * 4, 0xffffffff, "BUNDLE_RC_FACTOR_0", bundle_rc_factor_0, 0, 8);
		} else {
			ICREG(0x400e50 + i * 4, 0xffffffff, "BUNDLE_RC_FACTOR_1", bundle_rc_factor_1, 0, 8);
		}
		ICREG(0x400e58 + i * 4, 0x0003cfff, "BUNDLE_RC_OUT_ALPHA", bundle_rc_out_alpha, i, 8);
		ICREG(0x400e60 + i * 4, i ? 0x3803ffff : 0x0003ffff, "BUNDLE_RC_OUT_COLOR", bundle_rc_out_color, i, 8);
	}
	CREG(0x400e68, 0x3f3f3f3f, "BUNDLE_RC_FINAL_A", bundle_rc_final_a);
	CREG(0x400e6c, 0x3f3f3fe0, "BUNDLE_RC_FINAL_B", bundle_rc_final_b);
	CREG(0x400e70, is_nv17p ? 0xbfcf5fff : 0x3fcf5fff, "BUNDLE_CONFIG_A", bundle_config_a);
	CREG(0x400e74, 0xfffffff1, "BUNDLE_STENCIL_A", bundle_stencil_a);
	CREG(0x400e78, 0x00000fff, "BUNDLE_STENCIL_B", bundle_stencil_b);
	CREG(0x400e7c, is_nv17p ? 0x0000fff5 : 0x00003ff5, "BUNDLE_CONFIG_B", bundle_config_b);
	CREG(0x400e80, is_nv15p ? 0x0001ffff : 0x00000fff, "BUNDLE_BLEND", bundle_blend);
	CREG(0x400e84, 0xffffffff, "BUNDLE_BLEND_COLOR", bundle_blend_color);
	CREG(0x400e88, 0xfcffffcf, "BUNDLE_RASTER", bundle_raster);
	CREG(0x400e8c, 0xffffffff, "BUNDLE_FOG_COLOR", bundle_fog_color);
	CREG(0x400e90, 0xffffffff, "BUNDLE_POLYGON_OFFSET_FACTOR", bundle_polygon_offset_factor);
	CREG(0x400e94, 0xffffffff, "BUNDLE_POLYGON_OFFSET_UNITS", bundle_polygon_offset_units);
	CREG(0x400e98, 0xffffffff, "BUNDLE_DEPTH_RANGE_NEAR", bundle_depth_range_near);
	CREG(0x400e9c, 0xffffffff, "BUNDLE_DEPTH_RANGE_FAR", bundle_depth_range_far);
	ICREG(0x400ea0, 0xffffffff, "BUNDLE_TEX_COLOR_KEY", bundle_tex_color_key, 0, 0x10);
	ICREG(0x400ea4, 0xffffffff, "BUNDLE_TEX_COLOR_KEY", bundle_tex_color_key, 1, 0x10);
	CREG(0x400ea8, 0x000001ff, "BUNDLE_POINT_SIZE", bundle_point_size);
	if (is_nv17p) {
		ICREG(0x400eac, 0x0fff0fff, "BUNDLE_CLEAR_HV[0]", bundle_clear_hv, 0, 2);
		ICREG(0x400eb0, 0x0fff0fff, "BUNDLE_CLEAR_HV[1]", bundle_clear_hv, 1, 2);
		CREG(0x400eb4, 0x3fffffff, "BUNDLE_SURF_BASE_ZCULL", bundle_surf_base_zcull);
		CREG(0x400eb8, 0xbfffffff, "BUNDLE_SURF_LIMIT_ZCULL", bundle_surf_limit_zcull);
		CREG(0x400ebc, 0x3fffffff, "BUNDLE_SURF_OFFSET_ZCULL", bundle_surf_offset_zcull);
		CREG(0x400ec0, 0x0000ffff, "BUNDLE_SURF_PITCH_ZCULL", bundle_surf_pitch_zcull);
		CREG(0x400ec4, 0x07ffffff, "BUNDLE_SURF_BASE_CLIPID", bundle_surf_base_clipid);
		CREG(0x400ec8, 0x87ffffff, "BUNDLE_SURF_LIMIT_CLIPID", bundle_surf_limit_clipid);
		CREG(0x400ecc, 0x07ffffff, "BUNDLE_SURF_OFFSET_CLIPID", bundle_surf_offset_clipid);
		CREG(0x400ed0, 0x0000ffff, "BUNDLE_SURF_PITCH_CLIPID", bundle_surf_pitch_clipid);
		CREG(0x400ed4, 0x0000000f, "BUNDLE_CLIPID_ID", bundle_clipid_id);
		CREG(0x400ed8, 0x80000046, "BUNDLE_Z_CONFIG", bundle_z_config);
		CREG(0x400edc, 0xffffffff, "BUNDLE_CLEAR_ZETA", bundle_clear_zeta);
		CREG(0x400ee0, 0xffffffff, "CELSIUS_MTHD_UNK3FC", celsius_mthd_unk3fc);
	}
	return res;
}

class KelvinRegister : public MmioRegister {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		pgraph_kelvin_bundle(state, extr(addr, 2, 9), ref(state), true);
	}
	using MmioRegister::MmioRegister;
};

class SimpleKelvinRegister : public SimpleMmioRegister {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		pgraph_kelvin_bundle(state, extr(addr, 2, 9), ref(state), true);
	}
	using SimpleMmioRegister::SimpleMmioRegister;
};

template<int n>
class IndexedKelvinRegister : public IndexedMmioRegister<n> {
	void sim_write(struct pgraph_state *state, uint32_t val) override {
		MmioRegister::sim_write(state, val);
		uint32_t rval = IndexedMmioRegister<n>::ref(state);
		uint32_t ad = IndexedMmioRegister<n>::addr;
		pgraph_kelvin_bundle(state, extr(ad, 2, 9), rval, true);
	}
	using IndexedMmioRegister<n>::IndexedMmioRegister;
};

#define KREG(a, m, n, f) res.push_back(std::unique_ptr<Register>(new SimpleKelvinRegister(a, m, n, &pgraph_state::f)))
#define IKREGF(a, m, n, f, i, x, fx) res.push_back(std::unique_ptr<Register>(new IndexedKelvinRegister<x>(a, m, n, &pgraph_state::f, i, fx)))
#define IKREG(a, m, n, f, i, x) IKREGF(a, m, n, f, i, x, 0)

std::vector<std::unique_ptr<Register>> pgraph_kelvin_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	bool is_nv25p = nv04_pgraph_is_nv25p(&chipset);
	if (chipset.card_type == 0x20) {
		KREG(0x401800, 0xffff0111, "BUNDLE_MULTISAMPLE", bundle_multisample);
		KREG(0x401804, 0x0001ffff, "BUNDLE_BLEND", bundle_blend);
		KREG(0x401808, 0xffffffff, "BUNDLE_BLEND_COLOR", bundle_blend_color);
		for (int i = 0; i < 4; i++) {
			IKREG(0x40180c + i * 4, 0xffffffff, "BUNDLE_TEX_BORDER_COLOR", bundle_tex_border_color, i, 0x10);
		}
		for (int i = 0; i < 3; i++) {
			IKREG(0x40181c + i * 4, 0xffffffff, "BUNDLE_TEX_UNK10", bundle_tex_unk10, i, 3);
			IKREG(0x401828 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK11", bundle_tex_unk11, i, 3);
			IKREG(0x401834 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK13", bundle_tex_unk13, i, 3);
			IKREG(0x401840 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK12", bundle_tex_unk12, i, 3);
			IKREG(0x40184c + i * 4, 0xffffffff, "BUNDLE_TEX_UNK15", bundle_tex_unk15, i, 3);
			IKREG(0x401858 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK14", bundle_tex_unk14, i, 3);
		}
		for (int i = 0; i < 2; i++) {
			IKREG(0x401864 + i * 4, 0x0fff0fff, "BUNDLE_CLEAR_HV", bundle_clear_hv, i, 2);
		}
		KREG(0x40186c, 0xffffffff, "BUNDLE_CLEAR_COLOR", bundle_clear_color);
		for (int i = 0; i < 4; i++) {
			IKREG(0x401870 + i * 4, 0xffffffff, "BUNDLE_TEX_COLOR_KEY", bundle_tex_color_key, i, 0x10);
		}
	}
	for (int i = 0; i < 8; i++) {
		IKREG(0x401880 + i * 4, 0xffffffff, "BUNDLE_RC_FACTOR_0", bundle_rc_factor_0, i, 8);
		IKREG(0x4018a0 + i * 4, 0xffffffff, "BUNDLE_RC_FACTOR_1", bundle_rc_factor_1, i, 8);
		IKREG(0x4018c0 + i * 4, 0xffffffff, "BUNDLE_RC_IN_ALPHA", bundle_rc_in_alpha, i, 8);
		IKREG(0x401900 + i * 4, 0xffffffff, "BUNDLE_RC_IN_COLOR", bundle_rc_in_color, i, 8);
		if (chipset.card_type == 0x20) {
			if (!is_nv25p) {
				IKREG(0x4018e0 + i * 4, 0x0003cfff, "BUNDLE_RC_OUT_ALPHA", bundle_rc_out_alpha, i, 8);
				IKREG(0x401920 + i * 4, 0x000fffff, "BUNDLE_RC_OUT_COLOR", bundle_rc_out_color, i, 8);
			} else {
				IKREG(0x4018e0 + i * 4, 0x03c3cfff, "BUNDLE_RC_OUT_ALPHA", bundle_rc_out_alpha, i, 8);
				IKREG(0x401920 + i * 4, 0x03ffffff, "BUNDLE_RC_OUT_COLOR", bundle_rc_out_color, i, 8);
			}
		} else {
			IKREG(0x4018e0 + i * 4, 0x03c7cfff, "BUNDLE_RC_OUT_ALPHA", bundle_rc_out_alpha, i, 8);
			IKREG(0x401920 + i * 4, 0x07ffffff, "BUNDLE_RC_OUT_COLOR", bundle_rc_out_color, i, 8);
		}
	}
	if (chipset.card_type == 0x20) {
		if (!is_nv25p) {
			KREG(0x401940, 0x0001110f, "BUNDLE_RC_CONFIG", bundle_rc_config);
		} else {
			KREG(0x401940, 0x00f1110f, "BUNDLE_RC_CONFIG", bundle_rc_config);
		}
	} else {
		if (chipset.chipset != 0x34) {
			KREG(0x401940, 0x01f1110f, "BUNDLE_RC_CONFIG", bundle_rc_config);
		} else {
			KREG(0x401940, 0x06f1110f, "BUNDLE_RC_CONFIG", bundle_rc_config);
		}
	}
	KREG(0x401944, 0x3f3f3f3f, "BUNDLE_RC_FINAL_A", bundle_rc_final_a);
	KREG(0x401948, 0x3f3f3fe0, "BUNDLE_RC_FINAL_B", bundle_rc_final_b);
	if (!is_nv25p) {
		KREG(0x40194c, 0xffcf5fff, "BUNDLE_CONFIG_A", bundle_config_a);
	} else if (chipset.card_type < 0x40 && chipset.chipset != 0x34) {
		KREG(0x40194c, 0x3fcf5fff, "BUNDLE_CONFIG_A", bundle_config_a);
	} else if (chipset.card_type < 0x40) {
		KREG(0x40194c, 0xbfcf5fff, "BUNDLE_CONFIG_A", bundle_config_a);
	} else {
		KREG(0x40194c, 0x814f5f00, "BUNDLE_CONFIG_A", bundle_config_a);
	}
	if (chipset.card_type == 0x20) {
		KREG(0x401950, 0xfffffff1, "BUNDLE_STENCIL_A", bundle_stencil_a);
	} else {
		KREG(0x401950, 0xfffffff3, "BUNDLE_STENCIL_A", bundle_stencil_a);
	}
	if (chipset.card_type < 0x40 && chipset.chipset != 0x34) {
		KREG(0x401954, 0x00000fff, "BUNDLE_STENCIL_B", bundle_stencil_b);
	} else {
		KREG(0x401954, 0x0000ffff, "BUNDLE_STENCIL_B", bundle_stencil_b);
	}
	if (chipset.card_type == 0x20) {
		if (!is_nv25p) {
			KREG(0x401958, 0x00173fe5, "BUNDLE_CONFIG_B", bundle_config_b);
		} else {
			KREG(0x401958, 0xff173fff, "BUNDLE_CONFIG_B", bundle_config_b);
		}
	} else if (chipset.card_type == 0x30) {
		KREG(0x401958, 0xff1703ff, "BUNDLE_CONFIG_B", bundle_config_b);
	} else {
		KREG(0x401958, 0xff3fffff, "BUNDLE_CONFIG_B", bundle_config_b);
	}
	if (chipset.card_type >= 0x30) {
		KREG(0x40195c, 0x1fff1fff, "BUNDLE_VIEWPORT_OFFSET", bundle_viewport_offset);
		KREG(0x401960, 0xffffffc3, "BUNDLE_PS_OFFSET", bundle_ps_offset);
	}
	if (is_nv25p) {
		KREG(0x401964, 0x000000ff, "BUNDLE_CLIPID_ID", bundle_clipid_id);
		KREG(0x401968, 0x3fffffff, "BUNDLE_SURF_BASE_CLIPID", bundle_surf_base_clipid);
		KREG(0x40196c, 0xffffffff, "BUNDLE_SURF_LIMIT_CLIPID", bundle_surf_limit_clipid);
		KREG(0x401970, 0x3fffffff, "BUNDLE_SURF_OFFSET_CLIPID", bundle_surf_offset_clipid);
		KREG(0x401974, 0x0000ffff, "BUNDLE_SURF_PITCH_CLIPID", bundle_surf_pitch_clipid);
		KREG(0x401978, 0xffffff03, "BUNDLE_LINE_STIPPLE", bundle_line_stipple);
		if (chipset.card_type < 0x40) {
			KREG(0x40197c, 0x00000003, "BUNDLE_RT_ENABLE", bundle_rt_enable);
		} else {
			KREG(0x40197c, 0x000000ff, "BUNDLE_RT_ENABLE", bundle_rt_enable);
		}
	}
	KREG(0x401980, 0xffffffff, "BUNDLE_FOG_COLOR", bundle_fog_color);
	for (int i = 0; i < 2; i++) {
		IKREG(0x401984 + i * 4, 0xffffffff, "BUNDLE_FOG_COEFF", bundle_fog_coeff, i, 2);
	}
	if (!is_nv25p) {
		KREG(0x40198c, 0x000001ff, "BUNDLE_POINT_SIZE", bundle_point_size);
	} else {
		KREG(0x40198c, 0xffffffff, "BUNDLE_POINT_SIZE", bundle_point_size);
	}
	if (chipset.card_type == 0x20) {
		KREG(0x401990, 0xffffffff, "BUNDLE_RASTER", bundle_raster);
	} else if (chipset.card_type == 0x30) {
		KREG(0x401990, 0xfffffdff, "BUNDLE_RASTER", bundle_raster);
	} else {
		KREG(0x401990, 0xfffffdef, "BUNDLE_RASTER", bundle_raster);
	}
	if (chipset.card_type < 0x40) {
		KREG(0x401994, 0x0000ffff, "BUNDLE_TEX_SHADER_CULL_MODE", bundle_tex_shader_cull_mode);
		if (!is_nv25p) {
			KREG(0x401998, 0x003181ff, "BUNDLE_TEX_SHADER_MISC", bundle_tex_shader_misc);
			KREG(0x40199c, 0xc00fffef, "BUNDLE_TEX_SHADER_OP", bundle_tex_shader_op);
		} else {
			KREG(0x401998, 0xbf318fff, "BUNDLE_TEX_SHADER_MISC", bundle_tex_shader_misc);
			if (chipset.chipset != 0x34) {
				KREG(0x40199c, 0xe00fffff, "BUNDLE_TEX_SHADER_OP", bundle_tex_shader_op);
			} else {
				KREG(0x40199c, 0x000fffff, "BUNDLE_TEX_SHADER_OP", bundle_tex_shader_op);
			}
		}
	}
	KREG(0x4019a0, 0xffffffff, "BUNDLE_FENCE_OFFSET", bundle_fence_offset);
	if (chipset.card_type == 0x20) {
		if (!is_nv25p) {
			KREG(0x4019a4, 0x00000007, "BUNDLE_TEX_ZCOMP", bundle_tex_zcomp);
		} else {
			KREG(0x4019a4, 0x00000fff, "BUNDLE_TEX_ZCOMP", bundle_tex_zcomp);
		}
	} else {
		KREG(0x4019a4, 0x0fffffff, "BUNDLE_UNK069", bundle_unk069);
	}
	if (chipset.card_type < 0x40) {
		KREG(0x4019a8, 0xffffffff, "BUNDLE_UNK06A", bundle_unk06a);
	}
	for (int i = 0; i < 2; i++) {
		IKREG(0x4019ac + i * 4, 0xffffffff, "BUNDLE_RC_FINAL_FACTOR", bundle_rc_final_factor, i, 2);
	}
	for (int i = 0; i < 2; i++) {
		if (chipset.card_type == 0x20) {
			IKREG(0x4019b4 + i * 4, 0xffffffff, "BUNDLE_CLIP_HV", bundle_clip_hv, i, 2);
		} else {
			IKREG(0x4019b4 + i * 4, 0x1fff0fff, "BUNDLE_CLIP_HV", bundle_clip_hv, i, 2);
		}
	}
	if (chipset.card_type == 0x20) {
		for (int i = 0; i < 4; i++) {
			IKREG(0x4019bc + i * 4, 0x01171717, "BUNDLE_TEX_WRAP", bundle_tex_wrap, i, 0x10);
			IKREG(0x4019cc + i * 4, 0x7fffffff, "BUNDLE_TEX_CONTROL_A", bundle_tex_control_a, i, 0x10);
			IKREG(0x4019dc + i * 4, 0xffff0000, "BUNDLE_TEX_CONTROL_B", bundle_tex_control_b, i, 0x10);
		}
		for (int i = 0; i < 2; i++) {
			IKREG(0x4019ec + i * 4, 0xffffffff, "BUNDLE_TEX_CONTROL_C", bundle_tex_control_c, i, 2);
		}
		for (int i = 0; i < 4; i++) {
			IKREG(0x4019f4 + i * 4, 0xff3fffff, "BUNDLE_TEX_FILTER", bundle_tex_filter, i, 0x10);
			IKREG(0x401a04 + i * 4, 0xffff7ffe, "BUNDLE_TEX_FORMAT", bundle_tex_format, i, 0x10);
			IKREG(0x401a14 + i * 4, 0x1fff1fff, "BUNDLE_TEX_RECT", bundle_tex_rect, i, 0x10);
			IKREG(0x401a24 + i * 4, 0xffffffff, "BUNDLE_TEX_OFFSET", bundle_tex_offset, i, 0x10);
			IKREG(0x401a34 + i * 4, 0xffffffcd, "BUNDLE_TEX_PALETTE", bundle_tex_palette, i, 0x10);
		}
	} else {
		KREG(0x4019bc, 0xffff0111, "BUNDLE_MULTISAMPLE", bundle_multisample);
		if (chipset.card_type < 0x40) {
			for (int i = 0; i < 3; i++) {
				IKREG(0x4019c0 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK10", bundle_tex_unk10, i, 3);
				IKREG(0x4019cc + i * 4, 0xffffffff, "BUNDLE_TEX_UNK11", bundle_tex_unk11, i, 3);
				IKREG(0x4019d8 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK13", bundle_tex_unk13, i, 3);
				IKREG(0x4019e4 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK12", bundle_tex_unk12, i, 3);
				IKREG(0x4019f0 + i * 4, 0xffffffff, "BUNDLE_TEX_UNK15", bundle_tex_unk15, i, 3);
				IKREG(0x4019fc + i * 4, 0xffffffff, "BUNDLE_TEX_UNK14", bundle_tex_unk14, i, 3);
			}
		}
		if (chipset.card_type < 0x40)
			KREG(0x401a08, 0x0ff1ffff, "BUNDLE_BLEND", bundle_blend);
		else
			KREG(0x401a08, 0xfffffff7, "BUNDLE_BLEND", bundle_blend);
		KREG(0x401a0c, 0xffffffff, "BUNDLE_BLEND_COLOR", bundle_blend_color);
		for (int i = 0; i < 2; i++) {
			IKREG(0x401a10 + i * 4, 0x0fff0fff, "BUNDLE_CLEAR_HV", bundle_clear_hv, i, 2);
		}
		KREG(0x401a18, 0xffffffff, "BUNDLE_CLEAR_COLOR", bundle_clear_color);
		KREG(0x401a1c, 0x000fffff, "BUNDLE_STENCIL_C", bundle_stencil_c);
		KREG(0x401a20, 0x000fffff, "BUNDLE_STENCIL_D", bundle_stencil_d);
		KREG(0x401a24, 0x00333333, "BUNDLE_CLIP_PLANE_ENABLE", bundle_clip_plane_enable);
		for (int i = 0; i < 2; i++) {
			IKREG(0x401a2c + i * 4, 0x1fff0fff, "BUNDLE_VIEWPORT_HV", bundle_viewport_hv, i, 2);
		}
		for (int i = 0; i < 2; i++) {
			IKREG(0x401a34 + i * 4, 0x1fff0fff, "BUNDLE_SCISSOR_HV", bundle_scissor_hv, i, 2);
		}
	}
	for (int i = 0; i < 8; i++) {
		IKREG(0x401a44 + i * 4, 0x0fff0fff, "BUNDLE_CLIP_RECT_HORIZ", bundle_clip_rect_horiz, i, 8);
		IKREG(0x401a64 + i * 4, 0x0fff0fff, "BUNDLE_CLIP_RECT_VERT", bundle_clip_rect_vert, i, 8);
	}
	if (chipset.card_type == 0x20) {
		if (!is_nv25p) {
			KREG(0x401a84, 0x00000017, "BUNDLE_Z_CONFIG", bundle_z_config);
		} else {
			KREG(0x401a84, 0x00000057, "BUNDLE_Z_CONFIG", bundle_z_config);
		}
	} else if (chipset.card_type == 0x30) {
		if (chipset.chipset != 0x34) {
			KREG(0x401a84, 0xc00000d7, "BUNDLE_Z_CONFIG", bundle_z_config);
		} else {
			KREG(0x401a84, 0xc0000056, "BUNDLE_Z_CONFIG", bundle_z_config);
		}
	} else {
		KREG(0x401a84, 0x000000ff, "BUNDLE_Z_CONFIG", bundle_z_config);
	}
	KREG(0x401a88, 0xffffffff, "BUNDLE_CLEAR_ZETA", bundle_clear_zeta);
	KREG(0x401a8c, 0xffffffff, "BUNDLE_DEPTH_RANGE_FAR", bundle_depth_range_far);
	KREG(0x401a90, 0xffffffff, "BUNDLE_DEPTH_RANGE_NEAR", bundle_depth_range_near);
	for (int i = 0; i < 2; i++) {
		if (chipset.card_type < 0x40)
			IKREG(0x401a94 + i * 4, 0x0300ffff, "BUNDLE_DMA_TEX", bundle_dma_tex, i, 2);
		else
			IKREG(0x401a94 + i * 4, 0x03ffffff, "BUNDLE_DMA_TEX", bundle_dma_tex, i, 2);
	}
	for (int i = 0; i < 2; i++) {
		if (chipset.card_type < 0x40)
			IKREG(0x401a9c + i * 4, 0x0000ffff, "BUNDLE_DMA_VTX", bundle_dma_vtx, i, 2);
		else
			IKREG(0x401a9c + i * 4, 0x00ffffff, "BUNDLE_DMA_VTX", bundle_dma_vtx, i, 2);
	}
	KREG(0x401aa4, 0xffffffff, "BUNDLE_POLYGON_OFFSET_UNITS", bundle_polygon_offset_units);
	KREG(0x401aa8, 0xffffffff, "BUNDLE_POLYGON_OFFSET_FACTOR", bundle_polygon_offset_factor);
	if (chipset.card_type < 0x40) {
		for (int i = 0; i < 3; i++) {
			IKREG(0x401aac + i * 4, 0xffffffff, "BUNDLE_TEX_SHADER_CONST_EYE", bundle_tex_shader_const_eye, i, 3);
		}
	}
	if (is_nv25p && chipset.card_type == 0x20) {
		KREG(0x401ab8, 0x00000003, "BUNDLE_UNK0AE", bundle_unk0ae);
	}
	if (chipset.card_type >= 0x40) {
		// Probably different from the unk above, but...
		KREG(0x401ab8, 0xffffffff, "BUNDLE_UNK0AE", bundle_unk0ae);
	}
	if (chipset.card_type >= 0x30) {
		KREG(0x401abc, 0x00011a7f, "BUNDLE_UNK0AF", bundle_unk0af);
	}
	if (chipset.card_type >= 0x40) {
		KREG(0x401ac0, 0x03ffffff, "BUNDLE_DMA_UNK0B0", bundle_dma_unk0b0);
	}
	if (is_nv25p) {
		if (chipset.card_type < 0x40) {
			KREG(0x401ac0, 0x3fffffff, "BUNDLE_SURF_BASE_ZCULL", bundle_surf_base_zcull);
			KREG(0x401ac4, 0xffffffff, "BUNDLE_SURF_LIMIT_ZCULL", bundle_surf_limit_zcull);
			KREG(0x401ac8, 0x3fffffff, "BUNDLE_SURF_OFFSET_ZCULL", bundle_surf_offset_zcull);
			KREG(0x401acc, 0x0000ffff, "BUNDLE_SURF_PITCH_ZCULL", bundle_surf_pitch_zcull);
		}
		for (int i = 0; i < 4; i++) {
			IKREG(0x401ad0 + i * 4, 0xf8f8f8f8, "BUNDLE_UNK0B4", bundle_unk0b4, i, 4);
		}
		KREG(0x401ae0, 0x00000003, "BUNDLE_UNK0B8", bundle_unk0b8);
	}
	if (chipset.card_type >= 0x30) {
		KREG(0x401ae4, 0x00000001, "BUNDLE_PRIMITIVE_RESTART_ENABLE", bundle_primitive_restart_enable);
		KREG(0x401ae8, 0xffffffff, "BUNDLE_PRIMITIVE_RESTART_INDEX", bundle_primitive_restart_index);
		KREG(0x401aec, 0xffffffff, "BUNDLE_TXC_CYLWRAP", bundle_txc_cylwrap);
		if (chipset.card_type < 0x40)
			KREG(0x401b10, 0xf003ffff, "BUNDLE_PS_CONTROL", bundle_ps_control);
		else
			KREG(0x401b10, 0x7ff92afe, "BUNDLE_PS_CONTROL", bundle_ps_control);
		KREG(0x401b14, 0x000000ff, "BUNDLE_TXC_ENABLE", bundle_txc_enable);
		KREG(0x401b18, 0x00000001, "BUNDLE_UNK0C6", bundle_unk0c6);
		KREG(0x401b1c, 0x00003fff, "BUNDLE_WINDOW_CONFIG", bundle_window_config);
		if (chipset.card_type >= 0x40) {
			KREG(0x401b20, 0xffffffff, "BUNDLE_DEPTH_BOUNDS_MIN", bundle_depth_bounds_min);
			KREG(0x401b24, 0xffffffff, "BUNDLE_DEPTH_BOUNDS_MAX", bundle_depth_bounds_max);
			KREG(0x401b28, 0x17fffffd, "BUNDLE_XF_A", bundle_xf_a);
			KREG(0x401b2c, 0x0007ffff, "BUNDLE_XF_LIGHT", bundle_xf_light);
			KREG(0x401b30, 0xc80003ff, "BUNDLE_XF_C", bundle_xf_c);
			KREG(0x401b34, 0xffffffff, "BUNDLE_XF_ATTR_IN_MASK", bundle_xf_attr_in_mask);
			KREG(0x401b38, 0x003fffff, "BUNDLE_XF_ATTR_OUT_MASK", bundle_xf_attr_out_mask);
			KREG(0x401b3c, 0x000000ff, "BUNDLE_TXC_CYLWRAP_B", bundle_txc_cylwrap_b);
			for (int i = 0; i < 8; i++)
				IKREG(0x401b40 + i * 4, 0x000f7777, "BUNDLE_XF_TXC", bundle_xf_txc, i, 10);
			for (int i = 8; i < 10; i++)
				IKREG(0x401b40 + i * 4, 0x000e0000, "BUNDLE_XF_TXC", bundle_xf_txc, i, 10);
			KREG(0x401b68, 0xffffffff, "BUNDLE_XF_OUT_MAP_MISC_A", bundle_xf_out_map_misc_a);
			KREG(0x401b6c, 0xffffffff, "BUNDLE_XF_OUT_MAP_MISC_B", bundle_xf_out_map_misc_b);
			KREG(0x401b70, 0xffffffff, "BUNDLE_XF_OUT_MAP_TXC_A", bundle_xf_out_map_txc_a);
			KREG(0x401b74, 0x000000ff, "BUNDLE_XF_OUT_MAP_TXC_B", bundle_xf_out_map_txc_b);
			KREG(0x401b78, 0xffffffff, "BUNDLE_UNK0DE", bundle_unk0de);
			KREG(0x401b7c, 0xffffffff, "BUNDLE_VB_ELEMENT_BASE", bundle_vb_element_base);
			KREG(0x401b80, 0x000100ff, "BUNDLE_UNK0E0", bundle_unk0e0);
			KREG(0x401b84, 0xffffffff, "BUNDLE_UNK0E1", bundle_unk0e1);
			KREG(0x401b88, 0xffffffff, "BUNDLE_UNK0E2", bundle_unk0e2);
			KREG(0x401b8c, 0x0001ffff, "BUNDLE_XF_D", bundle_xf_d);
			KREG(0x401b90, 0x0000ffff, "BUNDLE_ALPHA_FUNC_REF", bundle_alpha_func_ref);
			KREG(0x401b94, 0xffffffff, "BUNDLE_UNK0E5", bundle_unk0e5);
			KREG(0x401b98, 0x01ffffff, "BUNDLE_UNK0E6", bundle_unk0e6);
			KREG(0x401b9c, 0x03ff03ff, "BUNDLE_XF_UNK0E7", bundle_xf_unk0e7);
			KREG(0x401ba0, 0x0fff0fff, "BUNDLE_XF_LOAD_POS", bundle_xf_load_pos);
			for (int i = 0; i < 4; i++)
				IKREG(0x401ba4 + i * 4, 0xffffffff, "BUNDLE_UNK0E9", bundle_unk0e9, i, 4);
			KREG(0x401bb4, 0xffffffff, "BUNDLE_UNK0ED", bundle_unk0ed);
			for (int i = 0; i < 4; i++) {
				IKREG(0x401bc0 + i * 4, 0x1fff0fff, "BUNDLE_CLIPID_RECT_HORIZ", bundle_clipid_rect_horiz, i, 4);
				IKREG(0x401bd0 + i * 4, 0x1fff0fff, "BUNDLE_CLIPID_RECT_VERT", bundle_clipid_rect_vert, i, 4);
			}
			KREG(0x401be0, 0x00ffff00, "BUNDLE_COLOR_MASK", bundle_color_mask);
		}
		for (int i = 0; i < 0x10; i++) {
			IKREG(0x401c00 + i * 4, 0xffffffff, "BUNDLE_TEX_OFFSET", bundle_tex_offset, i, 0x10);
			if (chipset.card_type < 0x40) {
				IKREG(0x401c40 + i * 4, 0xffff7fce, "BUNDLE_TEX_FORMAT", bundle_tex_format, i, 0x10);
				IKREG(0x401c80 + i * 4, 0xffff77f7, "BUNDLE_TEX_WRAP", bundle_tex_wrap, i, 0x10);
				IKREG(0x401cc0 + i * 4, 0x7fffffff, "BUNDLE_TEX_CONTROL_A", bundle_tex_control_a, i, 0x10);
				IKREG(0x401d00 + i * 4, 0xffffffff, "BUNDLE_TEX_CONTROL_B", bundle_tex_control_b, i, 0x10);
			} else {
				IKREG(0x401c40 + i * 4, 0xffffffce, "BUNDLE_TEX_FORMAT", bundle_tex_format, i, 0x10);
				IKREG(0x401c80 + i * 4, 0xfffffff7, "BUNDLE_TEX_WRAP", bundle_tex_wrap, i, 0x10);
				IKREG(0x401cc0 + i * 4, 0xffffffff, "BUNDLE_TEX_CONTROL_A", bundle_tex_control_a, i, 0x10);
				IKREG(0x401d00 + i * 4, 0x1fffffff, "BUNDLE_TEX_CONTROL_B", bundle_tex_control_b, i, 0x10);
			}
			IKREG(0x401d40 + i * 4, 0xff3fffff, "BUNDLE_TEX_FILTER", bundle_tex_filter, i, 0x10);
			IKREG(0x401d80 + i * 4, 0x1fff1fff, "BUNDLE_TEX_RECT", bundle_tex_rect, i, 0x10);
			IKREG(0x401dc0 + i * 4, 0xffffffff, "BUNDLE_TEX_BORDER_COLOR", bundle_tex_border_color, i, 0x10);
			if (chipset.card_type < 0x40) {
				IKREG(0x401e00 + i * 4, 0xffffffcd, "BUNDLE_TEX_PALETTE", bundle_tex_palette, i, 0x10);
			} else {
				IKREG(0x401e00 + i * 4, 0x3ff3ffff, "BUNDLE_TEX_CONTROL_D", bundle_tex_control_d, i, 0x10);
			}
			IKREG(0x401e40 + i * 4, 0xffffffff, "BUNDLE_TEX_COLOR_KEY", bundle_tex_color_key, i, 0x10);
		}
		if (chipset.card_type >= 0x40) {
			for (int i = 0; i < 4; i++) {
				IKREG(0x401e80 + i * 4, 0xffffffff, "BUNDLE_XF_TEX_OFFSET", bundle_xf_tex_offset, i, 4);
				IKREG(0x401e90 + i * 4, 0x000fffc2, "BUNDLE_XF_TEX_FORMAT", bundle_xf_tex_format, i, 4);
				IKREG(0x401ea0 + i * 4, 0x00000707, "BUNDLE_XF_TEX_WRAP", bundle_xf_tex_wrap, i, 4);
				IKREG(0x401eb0 + i * 4, 0x7fffffc0, "BUNDLE_XF_TEX_CONTROL_A", bundle_xf_tex_control_a, i, 4);
				IKREG(0x401ec0 + i * 4, 0x0003ffff, "BUNDLE_XF_TEX_CONTROL_B", bundle_xf_tex_control_b, i, 4);
				IKREG(0x401ed0 + i * 4, 0x00001fff, "BUNDLE_XF_TEX_FILTER", bundle_xf_tex_filter, i, 4);
				IKREG(0x401ee0 + i * 4, 0x1fff1fff, "BUNDLE_XF_TEX_RECT", bundle_xf_tex_rect, i, 4);
				IKREG(0x401ef0 + i * 4, 0xffffffff, "BUNDLE_XF_TEX_BORDER_COLOR", bundle_xf_tex_border_color, i, 4);
			}
		}
	}
	return res;
}

std::vector<std::unique_ptr<Register>> pgraph_dma_nv3_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	uint32_t offset_mask = pgraph_offset_mask(&chipset) | 0xf;
	REG(0x401140, 0x11111, "DMA_INTR_EN", dma_intr_en);
	IREG(0x401210, 0x03010fff, "DMA_ENG_FLAGS", dma_eng_flags, 0, 2);
	IREG(0x401220, 0xffffffff, "DMA_ENG_LIMIT", dma_eng_limit, 0, 2);
	IREG(0x401230, 0xfffff003, "DMA_ENG_PTE", dma_eng_pte, 0, 2);
	IREG(0x401240, 0xfffff000, "DMA_ENG_PTE_TAG", dma_eng_pte_tag, 0, 2);
	IREG(0x401250, 0xffffffff, "DMA_ENG_ADDR_VIRT_ADJ", dma_eng_addr_virt_adj, 0, 2);
	IREG(0x401260, 0xffffffff, "DMA_ENG_ADDR_PHYS", dma_eng_addr_phys, 0, 2);
	IREG(0x401270, offset_mask, "DMA_ENG_BYTES", dma_eng_bytes, 0, 2);
	IREG(0x401280, 0x0000ffff, "DMA_ENG_INST", dma_eng_inst, 0, 2);
	IREG(0x401290, 0x000007ff, "DMA_ENG_LINES", dma_eng_lines, 0, 2);
	REG(0x401400, offset_mask, "DMA_LIN_LIMIT", dma_lin_limit);
	IREG(0x401800, 0xffffffff, "DMA_OFFSET", dma_offset, 0, 3);
	IREG(0x401810, 0xffffffff, "DMA_OFFSET", dma_offset, 1, 3);
	IREG(0x401820, 0xffffffff, "DMA_OFFSET", dma_offset, 2, 3);
	REG(0x401830, 0xffffffff, "DMA_PITCH", dma_pitch);
	REG(0x401840, 0x00000707, "DMA_MISC", dma_misc);
	return res;
}

std::vector<std::unique_ptr<Register>> pgraph_dma_nv4_regs(const chipset_info &chipset) {
	std::vector<std::unique_ptr<Register>> res;
	bool is_nv17p = nv04_pgraph_is_nv17p(&chipset) && chipset.card_type < 0x20;
	IREG(0x401000, 0xffffffff, "DMA_OFFSET", dma_offset, 0, 3);
	IREG(0x401004, 0xffffffff, "DMA_OFFSET", dma_offset, 1, 3);
	REG(0x401008, 0x3fffff, "DMA_LENGTH", dma_length);
	REG(0x40100c, 0x77ffff, "DMA_MISC", dma_misc);
	if (chipset.card_type < 0x40) {
		IREG(0x401020, 0xffffffff, "DMA_UNK20", dma_unk20, 0, 2);
		IREG(0x401024, 0xffffffff, "DMA_UNK20", dma_unk20, 1, 2);
	}
	if (is_nv17p)
		REG(0x40103c, 0x1f, "DMA_UNK3C", dma_unk3c);
	if (chipset.card_type >= 0x20)
		REG(0x401038, 0xffffffff, "DMA_UNK38", dma_unk38);
	for (int i = 0; i < 2; i++) {
		if (chipset.card_type < 0x40) {
			IREG(0x401040 + i * 0x40, 0xffff, "DMA_ENG_INST", dma_eng_inst, i, 2);
		} else {
			IREG(0x401040 + i * 0x40, 0xffffff, "DMA_ENG_INST", dma_eng_inst, i, 2);
		}
		IREG(0x401044 + i * 0x40, 0xfff33000, "DMA_ENG_FLAGS", dma_eng_flags, i, 2);
		IREG(0x401048 + i * 0x40, 0xffffffff, "DMA_ENG_LIMIT", dma_eng_limit, i, 2);
		IREG(0x40104c + i * 0x40, 0xfffff002, "DMA_ENG_PTE", dma_eng_pte, i, 2);
		IREG(0x401050 + i * 0x40, 0xfffff000, "DMA_ENG_PTE_TAG", dma_eng_pte_tag, i, 2);
		IREG(0x401054 + i * 0x40, 0xffffffff, "DMA_ENG_ADDR_VIRT_ADJ", dma_eng_addr_virt_adj, i, 2);
		IREG(0x401058 + i * 0x40, 0xffffffff, "DMA_ENG_ADDR_PHYS", dma_eng_addr_phys, i, 2);
		IREG(0x40105c + i * 0x40, 0x1ffffff, "DMA_ENG_BYTES", dma_eng_bytes, i, 2);
		IREG(0x401060 + i * 0x40, 0x7ff, "DMA_ENG_LINES", dma_eng_lines, i, 2);
	}
	return res;
}

}
}

uint32_t gen_rnd(std::mt19937 &rnd) {
	uint32_t res = rnd();
	res = sext(res, rnd() & 0x1f);
	if (!(rnd() & 3)) {
		res ^= 1 << (rnd() & 0x1f);
	}
	return res;
}

using namespace hwtest::pgraph;

void pgraph_gen_state_debug(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_debug_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
	if (state->chipset.card_type >= 0x10) {
		// debug_a holds only resets.
		state->debug_a = 0;
	}
}

void pgraph_gen_state_fifo(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
	bool is_nv1720p = is_nv17p || state->chipset.card_type >= 0x20;
	if (state->chipset.card_type < 3) {
		state->access = rnd() & 0x0001f000;
		state->access |= 0x0f000111;
	} else if (state->chipset.card_type < 4) {
		state->fifo_enable = rnd() & 1;
	} else {
		state->fifo_enable = rnd() & 1;
		for (int i = 0; i < 8; i++) {
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd[i] = rnd() & 0x00007fff;
			} else if (!is_nv1720p) {
				state->fifo_mthd[i] = rnd() & 0x01171ffc;
			} else {
				state->fifo_mthd[i] = rnd() & 0x00371ffc;
			}
			state->fifo_data[i][0] = rnd() & 0xffffffff;
			state->fifo_data[i][1] = rnd() & 0xffffffff;
		}
		if (state->fifo_enable) {
			state->fifo_ptr = (rnd() & (is_nv1720p ? 0xf : 7)) * 0x11;
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd_st2 = rnd() & 0x000ffffe;
			} else if (state->chipset.card_type < 0x40) {
				state->fifo_mthd_st2 = rnd() & (is_nv1720p ? 0x7bf71ffc : 0x3bf71ffc);
			} else {
				state->fifo_mthd_st2 = rnd() & 0x42071ffc;
			}
		} else {
			state->fifo_ptr = rnd() & (is_nv1720p ? 0xff : 0x77);
			if (state->chipset.card_type < 0x10) {
				state->fifo_mthd_st2 = rnd() & 0x000fffff;
			} else if (state->chipset.card_type < 0x40) {
				state->fifo_mthd_st2 = rnd() & (is_nv1720p ? 0x7ff71ffc : 0x3ff71ffc);
			} else {
				state->fifo_mthd_st2 = rnd() & 0x46071ffc;
			}
		}
		state->fifo_data_st2[0] = rnd() & 0xffffffff;
		state->fifo_data_st2[1] = rnd() & 0xffffffff;
	}
	if (state->chipset.card_type < 4) {
		state->trap_addr = nva_rd32(cnum, 0x4006b4);
		state->trap_data[0] = nva_rd32(cnum, 0x4006b8);
		if (state->chipset.card_type == 3)
			state->trap_grctx = nva_rd32(cnum, 0x4006bc);
	} else {
		state->trap_addr = nva_rd32(cnum, 0x400704);
		state->trap_data[0] = nva_rd32(cnum, 0x400708);
		state->trap_data[1] = nva_rd32(cnum, 0x40070c);
	}
}

void pgraph_gen_state_control(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	bool is_nv15p = nv04_pgraph_is_nv15p(&state->chipset);
	for (auto &reg : pgraph_control_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
	state->intr = 0;
	state->invalid = 0;
	state->nsource = 0;
	if (state->chipset.card_type < 3) {
		state->pfb_config = rnd() & 0x1370;
		state->pfb_config |= nva_rd32(cnum, 0x600200) & ~0x1371;
		state->pfb_boot = nva_rd32(cnum, 0x600000);
	}
	if (state->chipset.card_type >= 0x10) {
		if (!is_nv15p) {
			state->state3d = rnd() & 0x1100ffff;
			if (state->state3d & 0x10000000)
				state->state3d |= 0x70000000;
		} else if (state->chipset.card_type < 0x20) {
			state->state3d = rnd() & 0x631fffff;
		} else if (state->chipset.card_type < 0x40) {
			state->state3d = rnd() & 0x0100ffff;
		}
		state->zcull_unka00[0] = rnd() & 0x1fff1fff;
		state->zcull_unka00[1] = rnd() & 0x1fff1fff;
		state->unka10 = rnd() & 0xdfff3fff;
	}
	if (state->chipset.card_type >= 4 && state->chipset.card_type < 0x40) {
		// XXX clean & duplicate for NV40
		if (!(rnd() & 3)) {
			insrt(state->ctx_switch_b, 16, 16, 0);
		}
		if (!(rnd() & 3)) {
			insrt(state->ctx_switch_c, 0, 16, 0);
		}
		if (!(rnd() & 3)) {
			insrt(state->ctx_switch_c, 16, 16, 0);
		}
		for (int i = 0; i < 8; i++) {
			if (!(rnd() & 3)) {
				insrt(state->ctx_cache_b[i], 16, 16, 0);
			}
			if (!(rnd() & 3)) {
				insrt(state->ctx_cache_c[i], 0, 16, 0);
			}
			if (!(rnd() & 3)) {
				insrt(state->ctx_cache_c[i], 16, 16, 0);
			}
		}
	}
	state->status = 0;
}

void pgraph_gen_state_vtx(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		state->vtx_xy[i][0] = gen_rnd(rnd);
		state->vtx_xy[i][1] = gen_rnd(rnd);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++)
			state->vtx_beta[i] = rnd() & 0x01ffffff;
	}
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = rnd() & 0x3ffff;
		for (int j = 0; j < 3; j++) {
			state->uclip_min[j][i] = rnd() & (state->chipset.card_type < 4 ? 0x3ffff : 0xffff);
			state->uclip_max[j][i] = rnd() & 0x3ffff;
		}
	}
}

void pgraph_gen_state_canvas(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_canvas_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
	if (state->chipset.card_type == 0x30) {
		// XXX figure this out
		state->surf_unk800 = 0;
	}
}

void pgraph_gen_state_rop(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_rop_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

void pgraph_gen_state_vstate(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_vstate_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

void pgraph_gen_state_dma_nv3(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->dma_intr = 0;
	for (auto &reg : pgraph_dma_nv3_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

void pgraph_gen_state_dma_nv4(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_dma_nv4_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

void pgraph_gen_state_d3d0(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d0_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

void pgraph_gen_state_d3d56(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d56_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

void pgraph_gen_state_fe3d(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_fe3d_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
}

static uint32_t canonical_light_sx_float(uint32_t v) {
	// 0
	if (!extr(v, 23, 8))
		return 0;
	// NaN
	if (extr(v, 23, 8) == 0xff)
		return v & 1 ? 0x7f800000 : 0x7ffffc00;
	return v & 0xfffffc00;
}

static uint32_t canonical_ovtx_fog(uint32_t v) {
	uint32_t res = v & 0xfffff800;
	if (res & 0x800)
		res |= 0x400;
	return res;
}

void pgraph_gen_state_celsius(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_celsius_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
	state->celsius_pipe_begin_end = rnd() & 0xf;
	state->celsius_pipe_edge_flag = rnd() & 0x1;
	state->celsius_pipe_unk48 = 0;
	state->celsius_pipe_vtx_state = rnd() & 0x70001f3f;
	for (int i = 0; i < 8; i++) {
		state->celsius_pipe_vtxbuf_offset[i] = rnd() & 0x0ffffffc;
		state->celsius_pipe_vtxbuf_format[i] = pgraph_celsius_fixup_vtxbuf_format(state, i, rnd());
		insrt(state->celsius_pipe_vtxbuf_format[i], 24, 1, extr(state->celsius_pipe_vtxbuf_format[0], 24, 1));
	}
	if (state->chipset.chipset == 0x10) {
		if (extr(state->celsius_pipe_vtxbuf_format[1], 0, 3) == 0)
			insrt(state->celsius_pipe_vtxbuf_format[2], 2, 1, 0);
		else
			insrt(state->celsius_pipe_vtxbuf_format[2], 2, 1, 1);
	}
	for (int i = 0; i < 0x1c; i++)
		state->celsius_pipe_vtx[i] = rnd();
	for (int i = 0; i < 4; i++)
		state->celsius_pipe_junk[i] = rnd();
	for (int i = 0; i < 0x3c; i++)
		for (int j = 0; j < 4; j++) {
			state->celsius_pipe_xfrm[i][j] = rnd();
		}
	for (int i = 0; i < 0x30; i++)
		for (int j = 0; j < 3; j++) {
			state->celsius_pipe_light_v[i][j] = rnd() & 0xfffffc00;
		}
	for (int i = 0; i < 3; i++)
		state->celsius_pipe_light_sa[i] = canonical_light_sx_float(rnd());
	for (int i = 0; i < 19; i++)
		state->celsius_pipe_light_sb[i] = canonical_light_sx_float(rnd());
	for (int i = 0; i < 12; i++)
		state->celsius_pipe_light_sc[i] = canonical_light_sx_float(rnd());
	for (int i = 0; i < 12; i++)
		state->celsius_pipe_light_sd[i] = canonical_light_sx_float(rnd());
	state->celsius_pipe_light_sa[0] = 0x3f800000;
	state->celsius_pipe_light_sb[0] = 0;
	state->celsius_pipe_light_sc[0] = 0x3f800000;
	state->celsius_pipe_light_sd[0] = 0;
	if (state->chipset.chipset == 0x10) {
		state->celsius_pipe_xfrm[59][0] = 0;
		state->celsius_pipe_xfrm[59][1] = 0;
		state->celsius_pipe_xfrm[59][2] = 0;
		state->celsius_pipe_xfrm[59][3] = 0;
		state->celsius_pipe_light_v[47][0] = 0;
		state->celsius_pipe_light_v[47][1] = 0;
		state->celsius_pipe_light_v[47][2] = 0;
		state->celsius_pipe_broke_ovtx = rnd() & 1;
	} else {
		state->celsius_pipe_broke_ovtx = false;
	}
	state->celsius_pipe_ovtx_pos = rnd() & 0xf;
	state->celsius_pipe_prev_ovtx_pos = 0xf;
	for (int i = 0; i < 3; i++) {
		// XXX clean this.
		state->celsius_pipe_xvtx[i][0] = rnd() & 0xbfffffff;
		state->celsius_pipe_xvtx[i][1] = rnd() & 0xbfffffff;
		state->celsius_pipe_xvtx[i][2] = 0;
		state->celsius_pipe_xvtx[i][3] = rnd() & 0xbfffffff;
		state->celsius_pipe_xvtx[i][4] = rnd() & 0xbfffffff;
		state->celsius_pipe_xvtx[i][5] = rnd() & 0xbfffffff;
		state->celsius_pipe_xvtx[i][6] = rnd() & 0x001ff000;
		state->celsius_pipe_xvtx[i][7] = rnd() & 0xbfffffff;
		state->celsius_pipe_xvtx[i][8] = 0;
		state->celsius_pipe_xvtx[i][9] = canonical_light_sx_float(rnd()) & ~0x400;
		state->celsius_pipe_xvtx[i][10] = rnd();
		state->celsius_pipe_xvtx[i][11] = rnd() & 0xffffff01;
		state->celsius_pipe_xvtx[i][12] = 0;
		state->celsius_pipe_xvtx[i][13] = 0;
		state->celsius_pipe_xvtx[i][14] = 0;
		state->celsius_pipe_xvtx[i][15] = 0x3f800000;
		/* If point params are disabled, point size comes from the global state.
		 * Since we have to load global state before pipe state, there's apparently
		 * no way to cheat here...  */
		if (!extr(state->bundle_config_b, 9, 1))
			state->celsius_pipe_xvtx[i][6] = state->bundle_point_size << 12;
		/* Same for specular enable.  */
		if (!extr(state->bundle_config_b, 5, 1))
			state->celsius_pipe_xvtx[i][11] &= 1;
	}
	for (int i = 0; i < 0x10; i++) {
		// XXX clean this.
		state->celsius_pipe_ovtx[i][0] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][1] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][2] = rnd() & 0x001ff000;
		state->celsius_pipe_ovtx[i][3] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][4] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][5] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][6] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][7] = rnd() & 0xbfffffff;
		state->celsius_pipe_ovtx[i][8] = rnd();
		state->celsius_pipe_ovtx[i][9] = canonical_light_sx_float(canonical_ovtx_fog(rnd()));
		state->celsius_pipe_ovtx[i][10] = rnd();
		state->celsius_pipe_ovtx[i][11] = rnd() & 0xffffff01;
		state->celsius_pipe_ovtx[i][12] = 0;
		state->celsius_pipe_ovtx[i][13] = 0;
		state->celsius_pipe_ovtx[i][14] = 0;
		state->celsius_pipe_ovtx[i][15] = 0x3f800000;
		insrt(state->celsius_pipe_ovtx[i][2], 31, 1, extr(state->celsius_pipe_ovtx[i][11], 0, 1));
	}
}

void pgraph_gen_state_kelvin(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	for (auto &reg : pgraph_kelvin_regs(state->chipset)) {
		reg->gen(state, cnum, rnd);
	}
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 0x200; j++)
			state->idx_cache[i][j] = rnd();
	for (int i = 0; i < 0x40; i++) {
		state->idx_fifo[i][0] = rnd();
		state->idx_fifo[i][1] = rnd();
		if (state->chipset.card_type == 0x20)
			state->idx_fifo[i][2] = rnd() & 0xffff;
		else
			state->idx_fifo[i][2] = rnd() & 0xfffff;
		state->idx_fifo[i][3] = 0;
		state->idx_prefifo[i][0] = rnd();
		state->idx_prefifo[i][1] = rnd();
		state->idx_prefifo[i][2] = rnd() & 0xfffff;
		state->idx_prefifo[i][3] = 0;
	}
	// XXX: Not controlled at the moment.
	if (state->chipset.card_type == 0x20) {
		state->idx_fifo_ptr = 41;
	} else {
		state->idx_fifo_ptr = 1;
		state->idx_prefifo_ptr = 3;
	}
	state->idx_unk27_ptr = 58;
	for (int i = 0; i < 0x80; i++)
		state->idx_unk25[i] = rnd() & 0xffffff;
	for (int i = 0; i < 0x10; i++) {
		state->idx_state_vtxbuf_offset[i] = rnd();
		state->idx_state_vtxbuf_format[i] = rnd() & 0x007fffff;
	}
	if (state->chipset.card_type == 0x20) {
		state->idx_state_a = rnd() & 0x01f0ffff;
	} else {
		state->idx_state_a = rnd() & 0x1f0fffff;
	}
	state->idx_state_b = rnd() & 0x1f1ffcfc;
	state->idx_state_c = rnd() & 0x013fffff;
	if (state->chipset.card_type == 0x30) {
		state->idx_state_d = rnd() & 0x7fffffff;
	}
	for (int i = 0; i < 0x100; i++)
		state->idx_unk27[i] = rnd() & 0xff;
	if (state->chipset.card_type == 0x20) {
		state->fd_state_begin_pt_a = rnd();
		state->fd_state_begin_pt_b = rnd();
		state->fd_state_begin_patch_c = rnd();
		state->fd_state_swatch = rnd();
		// XXX: Figure out how this can be safely unlocked...
		state->fd_state_unk10 = rnd() & 0xffffffff & 0xfffc0fff;
		state->fd_state_unk14 = rnd() & 0x7fffffff;
		state->fd_state_unk18 = rnd() & 0x0fffefff & 0xfffff7ff;
		state->fd_state_unk1c = rnd() & 0x3fffffff;
		state->fd_state_unk20 = rnd() & 0xffffffff;
		state->fd_state_unk24 = rnd() & 0x0fffffff;
		state->fd_state_unk28 = rnd() & 0x7fffffff;
		state->fd_state_unk2c = rnd() & 0x1fffffff;
		state->fd_state_unk30 = rnd() & 0x00ffffff;
		state->fd_state_unk34 = rnd() & 0x07ffffff & 0xfffffe0f;
		state->fd_state_begin_patch_d = rnd();
		// XXX wtf?
		insrt(state->fd_state_unk18, 6, 2, extr(state->fd_state_unk20, 4, 2));
		state->fd_state_unk10 |= extr(state->fd_state_unk34, 7, 4) << 13;
	}
	for (int i = 0; i < 0x11; i++)
		for (int j = 0; j < 4; j++)
			state->vab[i][j] = rnd();
	if (state->chipset.card_type == 0x20) {
		for (int i = 0; i < 0x88; i++) {
			state->xfpr[i][0] = rnd();
			state->xfpr[i][1] = rnd();
			state->xfpr[i][2] = rnd() & 0x0fffffff;
			state->xfpr[i][3] = 0;
		}
	} else if (state->chipset.card_type == 0x30) {
		for (int i = 0; i < 0x118; i++) {
			state->xfpr[i][0] = rnd();
			state->xfpr[i][1] = rnd();
			state->xfpr[i][2] = rnd();
			state->xfpr[i][3] = rnd() & 0x03ffffff;
		}
		for (int i = 0; i < 2; i++) {
			state->xfprunk1[i][0] = rnd();
			state->xfprunk1[i][1] = rnd();
			state->xfprunk1[i][2] = rnd() & 0x000000ff;
			state->xfprunk1[i][3] = 0;
		}
		state->xfprunk2 = rnd() & 0xffff;
	}
}

void pgraph_gen_state(int cnum, std::mt19937 &rnd, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	pgraph_gen_state_debug(cnum, rnd, state);
	pgraph_gen_state_fifo(cnum, rnd, state);
	pgraph_gen_state_control(cnum, rnd, state);
	pgraph_gen_state_vtx(cnum, rnd, state);
	pgraph_gen_state_canvas(cnum, rnd, state);
	pgraph_gen_state_rop(cnum, rnd, state);
	pgraph_gen_state_vstate(cnum, rnd, state);
	if (state->chipset.card_type == 3)
		pgraph_gen_state_dma_nv3(cnum, rnd, state);
	if (state->chipset.card_type >= 4)
		pgraph_gen_state_dma_nv4(cnum, rnd, state);
	pgraph_gen_state_fe3d(cnum, rnd, state);
	if (state->chipset.card_type == 3)
		pgraph_gen_state_d3d0(cnum, rnd, state);
	else if (state->chipset.card_type == 4)
		pgraph_gen_state_d3d56(cnum, rnd, state);
	else if (state->chipset.card_type == 0x10)
		pgraph_gen_state_celsius(cnum, rnd, state);
	else if (state->chipset.card_type >= 0x20)
		pgraph_gen_state_kelvin(cnum, rnd, state);
	pgraph_calc_state(state);
	state->shadow_config_b = state->bundle_config_b;
}

void pgraph_load_vtx(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x400450 + i * 4, state->iclip[i]);
			nva_wr32(cnum, 0x400460 + i * 8, state->uclip_min[0][i]);
			nva_wr32(cnum, 0x400464 + i * 8, state->uclip_max[0][i]);
		}
	} else {
		for (int i = 0; i < 2; i++) {
			nva_wr32(cnum, 0x400534 + i * 4, state->iclip[i]);
			nva_wr32(cnum, 0x40053c + i * 4, state->uclip_min[0][i]);
			nva_wr32(cnum, 0x400544 + i * 4, state->uclip_max[0][i]);
			nva_wr32(cnum, 0x400560 + i * 4, state->uclip_min[1][i]);
			nva_wr32(cnum, 0x400568 + i * 4, state->uclip_max[1][i]);
			if (state->chipset.card_type == 0x10) {
				nva_wr32(cnum, 0x400550 + i * 4, state->uclip_min[2][i]);
				nva_wr32(cnum, 0x400558 + i * 4, state->uclip_max[2][i]);
			}
		}
	}
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		nva_wr32(cnum, 0x400400 + i * 4, state->vtx_xy[i][0]);
		nva_wr32(cnum, 0x400480 + i * 4, state->vtx_xy[i][1]);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++)
			nva_wr32(cnum, 0x400700 + i * 4, state->vtx_beta[i]);
	}
}

void pgraph_load_rop(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_rop_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_canvas(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_canvas_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_control(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_control_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
	nva_wr32(cnum, 0x400100, 0xffffffff);
	if (state->chipset.card_type < 4) {
		nva_wr32(cnum, 0x400104, 0xffffffff);
	}

	if (state->chipset.card_type >= 0x10) {
		if (state->chipset.card_type < 0x40)
			nva_wr32(cnum, 0x40077c, state->state3d);
		bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
		if (is_nv17p) {
			nva_wr32(cnum, 0x400a00, state->zcull_unka00[0]);
			nva_wr32(cnum, 0x400a04, state->zcull_unka00[1]);
			nva_wr32(cnum, 0x400a10, state->unka10);
		}
	}
}

void pgraph_load_debug(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_debug_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
	if (state->chipset.card_type >= 0x10) {
		nva_wr32(cnum, 0x400080, state->debug_a);
	}
}

void pgraph_load_vstate(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_vstate_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_dma_nv3(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x401100, 0xffffffff);
	for (auto &reg : pgraph_dma_nv3_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_dma_nv4(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_dma_nv4_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_d3d0(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d0_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_d3d56(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d56_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_fe3d(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_fe3d_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
}

void pgraph_load_pipe(int cnum, uint32_t addr, uint32_t *data, int num) {
	nva_wr32(cnum, 0x400f50, addr);
	for (int i = 0; i < num; i++)
		nva_wr32(cnum, 0x400f54, data[i]);
	int ctr = 0;
	while (nva_rd32(cnum, 0x400700)) {
		ctr++;
		if (ctr == 10000) {
			printf("PIPE write hang on %04x: %08x\n", addr, nva_rd32(cnum, 0x400700));
		}
	}
}

void pgraph_load_celsius(int cnum, struct pgraph_state *state) {
	uint32_t ctr;
	if (nv04_pgraph_is_nv17p(&state->chipset))
		ctr = 9;
	else
		ctr = 11;
	while (ctr != state->celsius_pipe_ovtx_pos) {
		nva_wr32(cnum, 0x400e00, 0);
		ctr++;
		ctr &= 0xf;
	}
	for (auto &reg : pgraph_celsius_regs(state->chipset)) {
		reg->write(cnum, reg->ref(state));
	}
	nva_wr32(cnum, 0x400084, 0xffffffff);
	nva_wr32(cnum, 0x400088, 0xffffffff);
	nva_wr32(cnum, 0x40008c, 0xffffffff);
	nva_wr32(cnum, 0x400090, 0xffffffff);
	uint32_t cargo_a[8] = {
		0x3f800000,
		0x3f800000,
		0x3f800000,
		0x3f800000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
	};
	uint32_t cargo_b[3] = {
		0x3f800000,
		0x3f800000,
		0x3f800000,
	};
	uint32_t cargo_c[3] = { 0, 0, 0 };
	nva_wr32(cnum, 0x40014c, 0x00000056);
	nva_wr32(cnum, 0x40008c, 0);
	pgraph_load_pipe(cnum, 0x64c0, cargo_a, 8);
	pgraph_load_pipe(cnum, 0x6ab0, cargo_b, 3);
	pgraph_load_pipe(cnum, 0x6a80, cargo_c, 3);
	nva_wr32(cnum, 0x400f40, 0x10000000);
	nva_wr32(cnum, 0x400f44, 0x00000000);
	uint32_t mode = 0x8;
	pgraph_load_pipe(cnum, 0x0040, &mode, 1);
	for (int i = 0; i < 3; i++)
		pgraph_load_pipe(cnum, 0x0200 + i * 0x40, state->celsius_pipe_xvtx[i], 0x10);
	for (int i = 0; i < 0x10; i++)
		pgraph_load_pipe(cnum, 0x0800 + i * 0x40, state->celsius_pipe_ovtx[i], 0x10);
	uint32_t vtx[20];
	for (int i = 0; i < 8; i++) {
		vtx[i*2] = state->celsius_pipe_vtxbuf_offset[i];
		vtx[i*2+1] = state->celsius_pipe_vtxbuf_format[i];
	}
	vtx[16] = state->celsius_pipe_begin_end;
	vtx[17] = state->celsius_pipe_edge_flag;
	vtx[18] = state->celsius_pipe_unk48;
	vtx[19] = state->celsius_pipe_vtx_state;
	pgraph_load_pipe(cnum, 0x0000, vtx, 20);
	pgraph_load_pipe(cnum, 0x4400, state->celsius_pipe_vtx, 0x1c);
	for (int i = 0; i < 0x3c; i++) {
		pgraph_load_pipe(cnum, 0x6400 + i * 0x10, state->celsius_pipe_xfrm[i], 4);
	}
	for (int i = 0; i < 0x30; i++) {
		pgraph_load_pipe(cnum, 0x6800 + i * 0x10, state->celsius_pipe_light_v[i], 3);
	}
	for (int i = 0; i < 3; i++) {
		pgraph_load_pipe(cnum, 0x6c00 + i * 0x10, state->celsius_pipe_light_sa + i, 1);
	}
	for (int i = 0; i < 19; i++) {
		pgraph_load_pipe(cnum, 0x7000 + i * 0x10, state->celsius_pipe_light_sb + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_load_pipe(cnum, 0x7400 + i * 0x10, state->celsius_pipe_light_sc + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_load_pipe(cnum, 0x7800 + i * 0x10, state->celsius_pipe_light_sd + i, 1);
	}
	nva_wr32(cnum, 0x400f40, state->celsius_xf_misc_a);
	nva_wr32(cnum, 0x400f44, state->celsius_xf_misc_b);
	pgraph_load_pipe(cnum, 0x4470, state->celsius_pipe_junk, 0x4);
}

void pgraph_load_rdi(int cnum, uint32_t addr, uint32_t *data, int num) {
	nva_wr32(cnum, 0x400750, addr);
	for (int i = 0; i < num; i++)
		nva_wr32(cnum, 0x400754, data[i]);
	int ctr = 0;
	while (nva_rd32(cnum, 0x400700)) {
		ctr++;
		if (ctr == 10000) {
			printf("RDI write hang on %04x: %08x\n", addr, nva_rd32(cnum, 0x400700));
		}
	}
}

void pgraph_load_rdi4(int cnum, uint32_t addr, uint32_t (*data)[4], int num) {
	nva_wr32(cnum, 0x400750, addr);
	for (int i = 0; i < num; i++)
		for (int j = 0; j < 4; j++)
			nva_wr32(cnum, 0x400754, data[i][j]);
	int ctr = 0;
	while (nva_rd32(cnum, 0x400700)) {
		ctr++;
		if (ctr == 10000) {
			printf("RDI write hang on %04x: %08x\n", addr, nva_rd32(cnum, 0x400700));
		}
	}
}

void pgraph_load_rdi4_rev(int cnum, uint32_t addr, uint32_t (*data)[4], int num) {
	nva_wr32(cnum, 0x400750, addr);
	for (int i = 0; i < num; i++)
		for (int j = 0; j < 4; j++)
			nva_wr32(cnum, 0x400754, data[i][j ^ 3]);
	int ctr = 0;
	while (nva_rd32(cnum, 0x400700)) {
		ctr++;
		if (ctr == 10000) {
			printf("RDI write hang on %04x: %08x\n", addr, nva_rd32(cnum, 0x400700));
		}
	}
}

void pgraph_load_kelvin(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x40008c, 0);
	nva_wr32(cnum, 0x400090, 0);
	for (auto &reg : pgraph_kelvin_regs(state->chipset)) {
		while (nva_rd32(cnum, 0x400700));
		reg->write(cnum, reg->ref(state));
	}
	if (state->chipset.card_type == 0x20) {
		for (int i = 0; i < 4; i++)
			pgraph_load_rdi(cnum, (0x20 + i) << 16, state->idx_cache[i], 0x100);
	} else if (state->chipset.card_type == 0x30) {
		for (int i = 0; i < 2; i++)
			pgraph_load_rdi(cnum, (0x20 + i) << 16, state->idx_cache[i], 0x200);
	}
	pgraph_load_rdi4(cnum, 0x24 << 16, state->idx_fifo, 0x40);
	if (state->chipset.card_type == 0x30)
		pgraph_load_rdi4(cnum, 0x2ea << 16, state->idx_prefifo, 0x40);
	pgraph_load_rdi(cnum, 0x25 << 16, state->idx_unk25, 0x80);
	uint32_t idx_state[0x24];
	for (int i = 0; i < 0x10; i++) {
		idx_state[i * 2] = state->idx_state_vtxbuf_offset[i];
		idx_state[i * 2 + 1] = state->idx_state_vtxbuf_format[i];
	}
	idx_state[0x20] = state->idx_state_a;
	idx_state[0x21] = state->idx_state_b;
	idx_state[0x22] = state->idx_state_c;
	idx_state[0x23] = state->idx_state_d;
	if (state->chipset.card_type == 0x20) {
		pgraph_load_rdi(cnum, 0x26 << 16, idx_state, 0x23);
	} else if (state->chipset.card_type == 0x30) {
		pgraph_load_rdi(cnum, 0x26 << 16, idx_state, 0x24);
	}
	if (nv04_pgraph_is_nv25p(&state->chipset)) {
		uint32_t tmp[0x40];
		for (int i = 0; i < 0x100; i++) {
			insrt(tmp[i >> 2], (i & 3) * 8, 8, state->idx_unk27[i]);
		}
		pgraph_load_rdi(cnum, 0x27 << 16, tmp, 0x40);
	}
	if (state->chipset.card_type == 0x20) {
		uint32_t fd_state[0xf];
		fd_state[0x00] = state->fd_state_begin_pt_a;
		fd_state[0x01] = state->fd_state_begin_pt_b;
		fd_state[0x02] = state->fd_state_begin_patch_c;
		fd_state[0x03] = state->fd_state_swatch;
		fd_state[0x04] = state->fd_state_unk10;
		fd_state[0x05] = state->fd_state_unk14;
		fd_state[0x06] = state->fd_state_unk18;
		fd_state[0x07] = state->fd_state_unk1c;
		fd_state[0x08] = state->fd_state_unk20;
		fd_state[0x09] = state->fd_state_unk24;
		fd_state[0x0a] = state->fd_state_unk28;
		fd_state[0x0b] = state->fd_state_unk2c;
		fd_state[0x0c] = state->fd_state_unk30;
		fd_state[0x0d] = state->fd_state_unk34;
		fd_state[0x0e] = state->fd_state_begin_patch_d;
		pgraph_load_rdi(cnum, 0x3d << 16, fd_state, 0xf);
	}
	pgraph_load_rdi4_rev(cnum, 0x15 << 16, state->vab, 0x11);
	if (state->chipset.card_type == 0x20) {
		pgraph_load_rdi4(cnum, 0x10 << 16, state->xfpr, 0x88);
	} else {
		pgraph_load_rdi4(cnum, 0x10 << 16, state->xfpr, 0x118);
		pgraph_load_rdi4(cnum, 0x10 << 16 | 0x1180, state->xfprunk1, 2);
		uint32_t tmp[4];
		tmp[0] = state->xfprunk2;
		pgraph_load_rdi(cnum, 0x10 << 16 | 0x11a0, tmp, 4);
	}
}

void pgraph_load_fifo(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x4006a4, state->access);
	} else if (state->chipset.card_type < 4) {
		nva_wr32(cnum, 0x4006a4, state->fifo_enable);
	} else {
		if (state->chipset.card_type < 0x10) {
			for (int i = 0; i < 4; i++) {
				nva_wr32(cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
				nva_wr32(cnum, 0x400740 + i * 4, state->fifo_data[i][0]);
			}
			nva_wr32(cnum, 0x400750, state->fifo_ptr);
			nva_wr32(cnum, 0x400754, state->fifo_mthd_st2);
			nva_wr32(cnum, 0x400758, state->fifo_data_st2[0]);
		} else {
			bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
			bool is_nv1720p = is_nv17p || state->chipset.card_type >= 0x20;
			if (!is_nv1720p) {
				for (int i = 0; i < 4; i++) {
					nva_wr32(cnum, 0x400730 + i * 4, state->fifo_mthd[i]);
					nva_wr32(cnum, 0x400740 + i * 4, state->fifo_data[i][0]);
					nva_wr32(cnum, 0x400750 + i * 4, state->fifo_data[i][1]);
				}
			} else {
				for (int i = 0; i < 8; i++) {
					nva_wr32(cnum, 0x4007a0 + i * 4, state->fifo_mthd[i]);
					nva_wr32(cnum, 0x4007c0 + i * 4, state->fifo_data[i][0]);
					nva_wr32(cnum, 0x4007e0 + i * 4, state->fifo_data[i][1]);
				}
			}
			nva_wr32(cnum, 0x400760, state->fifo_ptr);
			nva_wr32(cnum, 0x400764, state->fifo_mthd_st2);
			nva_wr32(cnum, 0x400768, state->fifo_data_st2[0]);
			nva_wr32(cnum, 0x40076c, state->fifo_data_st2[1]);
		}
		nva_wr32(cnum, 0x400720, state->fifo_enable);
	}
}

void pgraph_load_state(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x000200, 0xffffeeff);
	nva_wr32(cnum, 0x000200, 0xffffffff);
	if (state->chipset.card_type < 3) {
		nva_wr32(cnum, 0x4006a4, 0x04000100);
		nva_wr32(cnum, 0x600200, state->pfb_config);
	} else if (state->chipset.card_type < 4) {
		nva_wr32(cnum, 0x4006a4, 0);
		for (int i = 0; i < 8; i++) {
			nva_wr32(cnum, 0x400648, 0x400 | i);
			nva_wr32(cnum, 0x40064c, 0xffffffff);
		}
	} else {
		nva_wr32(cnum, 0x400720, 0);
		if (state->chipset.card_type < 0x10)
			nva_wr32(cnum, 0x400160, 0);
		else
			nva_wr32(cnum, 0x40014c, 0);
	}

	pgraph_load_vtx(cnum, state);
	pgraph_load_rop(cnum, state);
	pgraph_load_fe3d(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_load_d3d0(cnum, state);
	else if (state->chipset.card_type == 4)
		pgraph_load_d3d56(cnum, state);
	else if (state->chipset.card_type == 0x10)
		pgraph_load_celsius(cnum, state);
	else if (state->chipset.card_type >= 0x20)
		pgraph_load_kelvin(cnum, state);

	pgraph_load_canvas(cnum, state);
	pgraph_load_vstate(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_load_dma_nv3(cnum, state);
	else if (state->chipset.card_type >= 4)
		pgraph_load_dma_nv4(cnum, state);

	pgraph_load_control(cnum, state);
	pgraph_load_debug(cnum, state);
	pgraph_load_fifo(cnum, state);
}

void pgraph_dump_fifo(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type < 4) {
		if (state->chipset.card_type < 3) {
			state->access = nva_rd32(cnum, 0x4006a4);
			nva_wr32(cnum, 0x4006a4, 0x04000100);
		} else {
			state->fifo_enable = nva_rd32(cnum, 0x4006a4);
			nva_wr32(cnum, 0x4006a4, 0);
		}
		state->trap_addr = nva_rd32(cnum, 0x4006b4);
		state->trap_data[0] = nva_rd32(cnum, 0x4006b8);
		if (state->chipset.card_type >= 3) {
			state->trap_grctx = nva_rd32(cnum, 0x4006bc);
		}
	} else {
		state->trap_addr = nva_rd32(cnum, 0x400704);
		state->trap_data[0] = nva_rd32(cnum, 0x400708);
		state->trap_data[1] = nva_rd32(cnum, 0x40070c);
		state->fifo_enable = nva_rd32(cnum, 0x400720);
		nva_wr32(cnum, 0x400720, 0);
		if (state->chipset.card_type < 0x10) {
			for (int i = 0; i < 4; i++) {
				state->fifo_mthd[i] = nva_rd32(cnum, 0x400730 + i * 4);
				state->fifo_data[i][0] = nva_rd32(cnum, 0x400740 + i * 4);
			}
			state->fifo_ptr = nva_rd32(cnum, 0x400750);
			state->fifo_mthd_st2 = nva_rd32(cnum, 0x400754);
			state->fifo_data_st2[0] = nva_rd32(cnum, 0x400758);
		} else {
			bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
			bool is_nv1720p = is_nv17p || state->chipset.card_type >= 0x20;
			if (!is_nv1720p) {
				for (int i = 0; i < 4; i++) {
					state->fifo_mthd[i] = nva_rd32(cnum, 0x400730 + i * 4);
					state->fifo_data[i][0] = nva_rd32(cnum, 0x400740 + i * 4);
					state->fifo_data[i][1] = nva_rd32(cnum, 0x400750 + i * 4);
				}
			} else {
				for (int i = 0; i < 8; i++) {
					state->fifo_mthd[i] = nva_rd32(cnum, 0x4007a0 + i * 4);
					state->fifo_data[i][0] = nva_rd32(cnum, 0x4007c0 + i * 4);
					state->fifo_data[i][1] = nva_rd32(cnum, 0x4007e0 + i * 4);
				}
			}
			state->fifo_ptr = nva_rd32(cnum, 0x400760);
			state->fifo_mthd_st2 = nva_rd32(cnum, 0x400764);
			state->fifo_data_st2[0] = nva_rd32(cnum, 0x400768);
			state->fifo_data_st2[1] = nva_rd32(cnum, 0x40076c);
		}
	}
}

void pgraph_dump_control(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_control_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
	if (state->chipset.card_type < 4) {
		state->intr = nva_rd32(cnum, 0x400100) & ~0x100;
		state->invalid = nva_rd32(cnum, 0x400104);
	} else {
		state->intr = nva_rd32(cnum, 0x400100);
		state->nsource = nva_rd32(cnum, 0x400108);
		if (state->chipset.card_type >= 0x10 && state->chipset.card_type < 0x40)
			state->state3d = nva_rd32(cnum, 0x40077c);
		bool is_nv17p = nv04_pgraph_is_nv17p(&state->chipset);
		if (is_nv17p) {
			state->zcull_unka00[0] = nva_rd32(cnum, 0x400a00);
			state->zcull_unka00[1] = nva_rd32(cnum, 0x400a04);
			state->unka10 = nva_rd32(cnum, 0x400a10);
		}
	}
}

void pgraph_dump_vstate(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_vstate_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_vtx(int cnum, struct pgraph_state *state) {
	for (int i = 0; i < pgraph_vtx_count(&state->chipset); i++) {
		state->vtx_xy[i][0] = nva_rd32(cnum, 0x400400 + i * 4);
		state->vtx_xy[i][1] = nva_rd32(cnum, 0x400480 + i * 4);
	}
	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++) {
			state->vtx_beta[i] = nva_rd32(cnum, 0x400700 + i * 4);
		}
	}

	if (state->chipset.card_type < 3) {
		for (int i = 0; i < 2; i++) {
			state->iclip[i] = nva_rd32(cnum, 0x400450 + i * 4);
			state->uclip_min[0][i] = nva_rd32(cnum, 0x400460 + i * 8);
			state->uclip_max[0][i] = nva_rd32(cnum, 0x400464 + i * 8);
		}
	} else {
		for (int i = 0; i < 2; i++) {
			state->iclip[i] = nva_rd32(cnum, 0x400534 + i * 4);
			state->uclip_min[0][i] = nva_rd32(cnum, 0x40053c + i * 4);
			state->uclip_max[0][i] = nva_rd32(cnum, 0x400544 + i * 4);
			state->uclip_min[1][i] = nva_rd32(cnum, 0x400560 + i * 4);
			state->uclip_max[1][i] = nva_rd32(cnum, 0x400568 + i * 4);
			if (state->chipset.card_type == 0x10) {
				state->uclip_min[2][i] = nva_rd32(cnum, 0x400550 + i * 4);
				state->uclip_max[2][i] = nva_rd32(cnum, 0x400558 + i * 4);
			}
		}
	}
}

void pgraph_dump_rop(int cnum, struct pgraph_state *state) {
	if (state->chipset.card_type == 4)
		nva_wr32(cnum, 0x40008c, 0x01000000);
	for (auto &reg : pgraph_rop_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_canvas(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_canvas_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_d3d0(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d0_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_d3d56(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_d3d56_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_fe3d(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_fe3d_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_pipe(int cnum, uint32_t addr, uint32_t *data, int num) {
	nva_wr32(cnum, 0x400f50, addr);
	for (int i = 0; i < num; i++)
		data[i] = nva_rd32(cnum, 0x400f54);
}

void pgraph_dump_celsius(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_celsius_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_celsius_pipe(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x40014c, 0x00000056);
	if (state->celsius_pipe_broke_ovtx)
		pgraph_dump_pipe(cnum, 0x4470, state->celsius_pipe_junk, 4);
	for (int i = 0; i < 0x10; i++)
		pgraph_dump_pipe(cnum, 0x0800 + i * 0x40, state->celsius_pipe_ovtx[i], 0x10);
	pgraph_dump_pipe(cnum, 0x4400, state->celsius_pipe_vtx, 0x1c);
	if (!state->celsius_pipe_broke_ovtx)
		pgraph_dump_pipe(cnum, 0x4470, state->celsius_pipe_junk, 4);
	for (int i = 0; i < 3; i++)
		pgraph_dump_pipe(cnum, 0x0200 + i * 0x40, state->celsius_pipe_xvtx[i], 0x10);
	for (int i = 0; i < 0x3c; i++) {
		pgraph_dump_pipe(cnum, 0x6400 + i * 0x10, state->celsius_pipe_xfrm[i], 4);
	}
	for (int i = 0; i < 0x30; i++) {
		pgraph_dump_pipe(cnum, 0x6800 + i * 0x10, state->celsius_pipe_light_v[i], 3);
	}
	for (int i = 0; i < 3; i++) {
		pgraph_dump_pipe(cnum, 0x6c00 + i * 0x10, state->celsius_pipe_light_sa + i, 1);
	}
	for (int i = 0; i < 19; i++) {
		pgraph_dump_pipe(cnum, 0x7000 + i * 0x10, state->celsius_pipe_light_sb + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_dump_pipe(cnum, 0x7400 + i * 0x10, state->celsius_pipe_light_sc + i, 1);
	}
	for (int i = 0; i < 12; i++) {
		pgraph_dump_pipe(cnum, 0x7800 + i * 0x10, state->celsius_pipe_light_sd + i, 1);
	}
	uint32_t vtx[20];
	pgraph_dump_pipe(cnum, 0x0000, vtx, 20);
	for (int i = 0; i < 8; i++) {
		state->celsius_pipe_vtxbuf_offset[i] = vtx[2*i];
		state->celsius_pipe_vtxbuf_format[i] = vtx[2*i+1];
	}
	state->celsius_pipe_begin_end = vtx[16];
	state->celsius_pipe_edge_flag = vtx[17];
	state->celsius_pipe_unk48 = vtx[18];
	state->celsius_pipe_vtx_state = vtx[19];
	if (state->chipset.chipset == 0x10) {
		// Fuck me with a chainsaw.  On NV10, vtxbuf_format readout is broken in
		// two ways:
		//
		// 1. If component count is 0 (ie. a vtxbuf is disabled), it reads as some
		//    non-zero value depending on the attribute instead.
		// 2. Bit 2 of format of attribute 2 (ie. secondary color) comes from
		//    attribute 1 (primary color) instead.
		//
		// Work around #1 by observing the last active attribute in vtx_state.
		for (unsigned i = 7; i > 0; i--) {
			uint32_t word = 0;
			pgraph_load_pipe(cnum, 4, &word, 1);
			pgraph_dump_pipe(cnum, 0x4c, &word, 1);
			if (extr(word, 0, 5) != i)
				insrt(state->celsius_pipe_vtxbuf_format[i], 4, 3, 0);
			word = 0;
			pgraph_load_pipe(cnum, i*8+4, &word, 1);
		}
	}
}

void pgraph_dump_rdi(int cnum, uint32_t addr, uint32_t *data, int num) {
	nva_wr32(cnum, 0x400750, addr);
	for (int i = 0; i < num; i++)
		data[i] = nva_rd32(cnum, 0x400754);
}

void pgraph_dump_rdi4(int cnum, uint32_t addr, uint32_t (*data)[4], int num) {
	nva_wr32(cnum, 0x400750, addr);
	for (int i = 0; i < num; i++)
		for (int j = 0; j < 4; j++)
			data[i][j] = nva_rd32(cnum, 0x400754);
}

void pgraph_dump_rdi4_rev(int cnum, uint32_t addr, uint32_t (*data)[4], int num) {
	nva_wr32(cnum, 0x400750, addr);
	for (int i = 0; i < num; i++)
		for (int j = 0; j < 4; j++)
			data[i][j ^ 3] = nva_rd32(cnum, 0x400754);
}

void pgraph_dump_kelvin(int cnum, struct pgraph_state *state) {
	nva_wr32(cnum, 0x40008c, 0);
	nva_wr32(cnum, 0x400090, 0);
	for (auto &reg : pgraph_kelvin_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
	if (state->chipset.card_type == 0x20) {
		for (int i = 0; i < 4; i++)
			pgraph_dump_rdi(cnum, (0x20 + i) << 16, state->idx_cache[i], 0x100);
	} else {
		for (int i = 0; i < 2; i++)
			pgraph_dump_rdi(cnum, (0x20 + i) << 16, state->idx_cache[i], 0x200);
	}
	pgraph_dump_rdi4(cnum, 0x24 << 16, state->idx_fifo, 0x40);
	if (state->chipset.card_type == 0x30)
		pgraph_dump_rdi4(cnum, 0x2ea << 16, state->idx_prefifo, 0x40);
	pgraph_dump_rdi(cnum, 0x25 << 16, state->idx_unk25, 0x80);
	uint32_t idx_state[0x24];
	if (state->chipset.card_type == 0x20) {
		pgraph_dump_rdi(cnum, 0x26 << 16, idx_state, 0x23);
	} else {
		pgraph_dump_rdi(cnum, 0x26 << 16, idx_state, 0x24);
	}
	for (int i = 0; i < 0x10; i++) {
		state->idx_state_vtxbuf_offset[i] = idx_state[i * 2];
		state->idx_state_vtxbuf_format[i] = idx_state[i * 2 + 1];
	}
	if (state->chipset.card_type == 0x20) {
		// XXX model that thing
		state->idx_state_a = idx_state[0x20] & ~0xf0000;
	} else {
		state->idx_state_a = idx_state[0x20];
	}
	state->idx_state_b = idx_state[0x21];
	state->idx_state_c = idx_state[0x22];
	state->idx_state_d = idx_state[0x23];
	if (nv04_pgraph_is_nv25p(&state->chipset)) {
		uint32_t tmp[0x40];
		pgraph_dump_rdi(cnum, 0x27 << 16, tmp, 0x40);
		for (int i = 0; i < 0x100; i++) {
			state->idx_unk27[i] = extr(tmp[i >> 2], (i & 3) * 8, 8);
		}
	}
	if (state->chipset.card_type == 0x20) {
		uint32_t fd_state[0xf];
		pgraph_dump_rdi(cnum, 0x3d << 16, fd_state, 0xf);
		state->fd_state_begin_pt_a = fd_state[0x00];
		state->fd_state_begin_pt_b = fd_state[0x01];
		state->fd_state_begin_patch_c = fd_state[0x02];
		state->fd_state_swatch = fd_state[0x03];
		state->fd_state_unk10 = fd_state[0x04];
		state->fd_state_unk14 = fd_state[0x05];
		state->fd_state_unk18 = fd_state[0x06];
		state->fd_state_unk1c = fd_state[0x07];
		state->fd_state_unk20 = fd_state[0x08];
		state->fd_state_unk24 = fd_state[0x09];
		state->fd_state_unk28 = fd_state[0x0a];
		state->fd_state_unk2c = fd_state[0x0b];
		state->fd_state_unk30 = fd_state[0x0c];
		state->fd_state_unk34 = fd_state[0x0d];
		state->fd_state_begin_patch_d = fd_state[0x0e];
	}
	pgraph_dump_rdi4_rev(cnum, 0x15 << 16, state->vab, 0x11);
	if (state->chipset.card_type == 0x20) {
		pgraph_dump_rdi4(cnum, 0x10 << 16, state->xfpr, 0x88);
	} else {
		pgraph_dump_rdi4(cnum, 0x10 << 16, state->xfpr, 0x118);
		pgraph_dump_rdi4(cnum, 0x10 << 16 | 0x1180, state->xfprunk1, 2);
		uint32_t tmp[4];
		pgraph_dump_rdi(cnum, 0x10 << 16 | 0x11a0, tmp, 4);
		state->xfprunk2 = tmp[0];
	}
}

void pgraph_dump_debug(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_debug_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
	if (state->chipset.card_type >= 0x10) {
		state->debug_a = nva_rd32(cnum, 0x400080);
		nva_wr32(cnum, 0x400080, 0);
	}
}

void pgraph_dump_dma_nv3(int cnum, struct pgraph_state *state) {
	state->dma_intr = nva_rd32(cnum, 0x401100);
	for (auto &reg : pgraph_dma_nv3_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_dma_nv4(int cnum, struct pgraph_state *state) {
	for (auto &reg : pgraph_dma_nv4_regs(state->chipset)) {
		reg->ref(state) = reg->read(cnum);
	}
}

void pgraph_dump_state(int cnum, struct pgraph_state *state) {
	state->chipset = nva_cards[cnum]->chipset;
	int ctr = 0;
	bool reset = false;
	while((state->status = nva_rd32(cnum, state->chipset.card_type < 4 ? 0x4006b0: 0x400700))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
			reset = true;
			break;
		}
	}
	pgraph_dump_debug(cnum, state);
	pgraph_dump_fifo(cnum, state);
	pgraph_dump_control(cnum, state);
	pgraph_dump_vstate(cnum, state);
	pgraph_dump_rop(cnum, state);
	pgraph_dump_canvas(cnum, state);
	pgraph_dump_fe3d(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_dump_d3d0(cnum, state);
	else if (state->chipset.card_type == 4)
		pgraph_dump_d3d56(cnum, state);
	else if (state->chipset.card_type == 0x10)
		pgraph_dump_celsius(cnum, state);
	else if (state->chipset.card_type >= 0x20)
		pgraph_dump_kelvin(cnum, state);

	if (state->chipset.card_type == 3)
		pgraph_dump_dma_nv3(cnum, state);
	else if (state->chipset.card_type >= 4)
		pgraph_dump_dma_nv4(cnum, state);

	if (reset) {
		nva_wr32(cnum, 0x000200, 0xffffefff);
		nva_wr32(cnum, 0x000200, 0xffffffff);
	}

	if (state->chipset.card_type == 0x10)
		pgraph_dump_celsius_pipe(cnum, state);

	pgraph_dump_vtx(cnum, state);
}

void pgraph_calc_state(struct pgraph_state *state) {
	if (nv04_pgraph_is_nv17p(&state->chipset)) {
		insrt(state->unka10, 29, 1, extr(state->debug_e, 2, 1) && !!extr(state->surf_type, 2, 2));
	}
}

int pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke) {
	bool is_nv17p = nv04_pgraph_is_nv17p(&orig->chipset);
	bool is_nv1720p = is_nv17p || orig->chipset.card_type >= 0x20;
	bool is_nv25p = nv04_pgraph_is_nv25p(&orig->chipset);
	bool print = false;
#define CMP(reg, name, ...) \
	if (print) \
		printf("%08x %08x %08x " name " %s\n", \
			orig->reg, exp->reg, real->reg , \
			## __VA_ARGS__, (exp->reg == real->reg ? "" : "*")); \
	else if (exp->reg != real->reg) { \
		printf("Difference in reg " name ": expected %08x real %08x\n" , \
		## __VA_ARGS__, exp->reg, real->reg); \
		broke = true; \
	}
#define CMP_LIST(list) \
	do { \
		for (auto &reg : list) { \
			std::string name = reg->name(); \
			if (print) \
				printf("%08x %08x %08x %s %s\n", \
					reg->ref(orig), reg->ref(exp), reg->ref(real), name.c_str(), \
					(!reg->diff(exp, real) ? "" : "*")); \
			else if (reg->diff(exp, real)) { \
				printf("Difference in reg %s: expected %08x real %08x\n", \
					name.c_str(), reg->ref(exp), reg->ref(real)); \
				broke = true; \
			} \
		} \
	} while (0)
restart:
	CMP(status, "STATUS")
	if (orig->chipset.card_type < 4) {
		// XXX: figure these out someday
#if 0
		CMP(trap_addr, "TRAP_ADDR")
		CMP(trap_data[0], "TRAP_DATA[0]")
		if (orig->chipset.card_type >= 3) {
			CMP(trap_grctx, "TRAP_GRCTX")
		}
#endif
	} else {
		CMP(trap_addr, "TRAP_ADDR")
		CMP(trap_data[0], "TRAP_DATA[0]")
		if (orig->chipset.card_type >= 0x10) {
			CMP(trap_data[1], "TRAP_DATA[1]")
		}
	}
	CMP_LIST(pgraph_debug_regs(orig->chipset));
	if (orig->chipset.card_type >= 0x10) {
		CMP(debug_a, "DEBUG_A")
	}

	if (orig->chipset.card_type < 3) {
		CMP(access, "ACCESS")
	} else {
		CMP(fifo_enable, "FIFO_ENABLE")
	}

	if (orig->chipset.card_type >= 4) {
		if (orig->chipset.card_type >= 0x10 && orig->chipset.card_type < 0x40) {
			CMP(state3d, "STATE3D")
		}
		if (is_nv17p) {
			CMP(zcull_unka00[0], "ZCULL_UNKA00[0]")
			CMP(zcull_unka00[1], "ZCULL_UNKA00[1]")
			CMP(unka10, "UNKA10")
		}
		for (int i = 0; i < (is_nv1720p ? 8 : 4); i++) {
			CMP(fifo_mthd[i], "FIFO_MTHD[%d]", i)
			CMP(fifo_data[i][0], "FIFO_DATA[%d][0]", i)
			if (orig->chipset.card_type >= 0x10) {
				CMP(fifo_data[i][1], "FIFO_DATA[%d][1]", i)
			}
		}
		CMP(fifo_ptr, "FIFO_PTR")
		CMP(fifo_mthd_st2, "FIFO_MTHD_ST2")
		CMP(fifo_data_st2[0], "FIFO_DATA_ST2[0]")
		if (orig->chipset.card_type >= 0x10) {
			CMP(fifo_data_st2[1], "FIFO_DATA_ST2[1]")
		}
	}

	CMP(intr, "INTR")
	if (orig->chipset.card_type < 4) {
		CMP(invalid, "INVALID")
	} else {
		CMP(nsource, "NSOURCE")
	}
	CMP_LIST(pgraph_control_regs(orig->chipset));

	// VTX
	for (int i = 0; i < 2; i++) {
		CMP(iclip[i], "ICLIP[%d]", i)
	}
	for (int i = 0; i < 2; i++) {
		CMP(uclip_min[0][i], "UCLIP_MIN[%d]", i)
		CMP(uclip_max[0][i], "UCLIP_MAX[%d]", i)
	}
	if (orig->chipset.card_type >= 3) {
		for (int i = 0; i < 2; i++) {
			CMP(uclip_min[1][i], "OCLIP_MIN[%d]", i)
			CMP(uclip_max[1][i], "OCLIP_MAX[%d]", i)
		}
	}
	if (orig->chipset.card_type == 0x10) {
		for (int i = 0; i < 2; i++) {
			CMP(uclip_min[2][i], "CLIP3D_MIN[%d]", i)
			CMP(uclip_max[2][i], "CLIP3D_MAX[%d]", i)
		}
	}
	for (int i = 0; i < pgraph_vtx_count(&orig->chipset); i++) {
		CMP(vtx_xy[i][0], "VTX_X[%d]", i)
		CMP(vtx_xy[i][1], "VTX_Y[%d]", i)
	}
	if (orig->chipset.card_type < 3) {
		for (int i = 0; i < 14; i++) {
			CMP(vtx_beta[i], "VTX_BETA[%d]", i)
		}
	}

	// VSTATE
	CMP_LIST(pgraph_vstate_regs(orig->chipset));
	CMP_LIST(pgraph_rop_regs(orig->chipset));
	CMP_LIST(pgraph_canvas_regs(orig->chipset));
	CMP_LIST(pgraph_fe3d_regs(orig->chipset));

	if (orig->chipset.card_type == 3) {
		CMP_LIST(pgraph_d3d0_regs(orig->chipset));
	} else if (orig->chipset.card_type == 4) {
		CMP_LIST(pgraph_d3d56_regs(orig->chipset));
	} else if (orig->chipset.card_type == 0x10) {
		CMP_LIST(pgraph_celsius_regs(orig->chipset));
		for (int i = 0; i < 8; i++) {
			CMP(celsius_pipe_vtxbuf_offset[i], "CELSIUS_PIPE_VTXBUF_OFFSET[%d]", i)
			CMP(celsius_pipe_vtxbuf_format[i], "CELSIUS_PIPE_VTXBUF_FORMAT[%d]", i)
		}
		CMP(celsius_pipe_begin_end, "CELSIUS_PIPE_BEGIN_END")
		CMP(celsius_pipe_edge_flag, "CELSIUS_PIPE_EDGE_FLAG")
		CMP(celsius_pipe_unk48, "CELSIUS_PIPE_UNK48")
		CMP(celsius_pipe_vtx_state, "CELSIUS_PIPE_VTX_STATE")
		for (int i = 0; i < 0x1c; i++) {
			CMP(celsius_pipe_vtx[i], "CELSIUS_PIPE_VTX[%d]", i)
		}
		if (real->celsius_pipe_broke_ovtx || real->chipset.chipset != 0x10) {
			for (int i = 0; i < 4; i++)
				CMP(celsius_pipe_junk[i], "CELSIUS_PIPE_JUNK[%d]", i)
		}
		for (int i = 0; i < 0x3c; i++) {
			for (int j = 0; j < 4; j++)
				CMP(celsius_pipe_xfrm[i][j], "CELSIUS_PIPE_XFRM[%d][%d]", i, j)
		}
		for (int i = 0; i < 0x30; i++) {
			for (int j = 0; j < 3; j++)
				CMP(celsius_pipe_light_v[i][j], "CELSIUS_PIPE_LIGHT_V[%d][%d]", i, j)
		}
		for (int i = 0; i < 3; i++) {
			CMP(celsius_pipe_light_sa[i], "CELSIUS_PIPE_LIGHT_SA[%d]", i)
		}
		for (int i = 0; i < 19; i++) {
			CMP(celsius_pipe_light_sb[i], "CELSIUS_PIPE_LIGHT_SB[%d]", i)
		}
		for (int i = 0; i < 12; i++) {
			CMP(celsius_pipe_light_sc[i], "CELSIUS_PIPE_LIGHT_SC[%d]", i)
		}
		for (int i = 0; i < 12; i++) {
			CMP(celsius_pipe_light_sd[i], "CELSIUS_PIPE_LIGHT_SD[%d]", i)
		}
		for (int i = 0; i < 0x3; i++) {
			for (int j = 0; j < 0x10; j++)
				CMP(celsius_pipe_xvtx[i][j], "CELSIUS_PIPE_XVTX[%d][%d]", i, j)
		}
		if (print) {
			CMP(celsius_pipe_ovtx_pos, "CELSIUS_PIPE_OVTX_POS")
			CMP(celsius_pipe_prev_ovtx_pos, "CELSIUS_PIPE_PREV_OVTX_POS")
		}
		for (int i = 0; i < 0x10; i++) {
			CMP(celsius_pipe_ovtx[i][0], "CELSIUS_PIPE_OVTX[%d].TXC1.S", i)
			CMP(celsius_pipe_ovtx[i][1], "CELSIUS_PIPE_OVTX[%d].TXC1.T", i)
			CMP(celsius_pipe_ovtx[i][2], "CELSIUS_PIPE_OVTX[%d].PTSZ", i)
			CMP(celsius_pipe_ovtx[i][3], "CELSIUS_PIPE_OVTX[%d].TXC1.Q", i)
			if (!real->celsius_pipe_broke_ovtx || i != 0) {
				CMP(celsius_pipe_ovtx[i][4], "CELSIUS_PIPE_OVTX[%d].TXC0.S", i)
				CMP(celsius_pipe_ovtx[i][5], "CELSIUS_PIPE_OVTX[%d].TXC0.T", i)
				CMP(celsius_pipe_ovtx[i][7], "CELSIUS_PIPE_OVTX[%d].TXC0.Q", i)
			}
			CMP(celsius_pipe_ovtx[i][9], "CELSIUS_PIPE_OVTX[%d].FOG", i)
			CMP(celsius_pipe_ovtx[i][10], "CELSIUS_PIPE_OVTX[%d].COL0", i)
			CMP(celsius_pipe_ovtx[i][11], "CELSIUS_PIPE_OVTX[%d].COL1E", i)
			CMP(celsius_pipe_ovtx[i][12], "CELSIUS_PIPE_OVTX[%d].POS.X", i)
			CMP(celsius_pipe_ovtx[i][13], "CELSIUS_PIPE_OVTX[%d].POS.Y", i)
			CMP(celsius_pipe_ovtx[i][14], "CELSIUS_PIPE_OVTX[%d].POS.Z", i)
			CMP(celsius_pipe_ovtx[i][15], "CELSIUS_PIPE_OVTX[%d].POS.W", i)
		}
	} else if (orig->chipset.card_type >= 0x20) {
		CMP_LIST(pgraph_kelvin_regs(orig->chipset));
		if (orig->chipset.card_type < 0x40) {
			if (orig->chipset.card_type == 0x30) {
				for (int i = 0; i < 0x40; i++) {
					for (int j = 0; j < 4; j++) {
						CMP(idx_prefifo[i][j], "IDX_PREFIFO[%d][%d]", i, j)
					}
				}
			}
			if (orig->chipset.card_type == 0x20) {
				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 0x100; j++) {
						CMP(idx_cache[i][j], "IDX_CACHE[%d][%d]", i, j)
					}
			} else {
				for (int i = 0; i < 2; i++)
					for (int j = 0; j < 0x200; j++) {
						CMP(idx_cache[i][j], "IDX_CACHE[%d][%d]", i, j)
					}
			}
			for (int i = 0; i < 0x40; i++) {
				for (int j = 0; j < 4; j++) {
					CMP(idx_fifo[i][j], "IDX_FIFO[%d][%d]", i, j)
				}
			}
			for (int i = 0; i < 0x80; i++) {
				CMP(idx_unk25[i], "IDX_UNK25[%d]", i)
			}
			if (is_nv25p) {
				for (int i = 0; i < 0x100; i++) {
					CMP(idx_unk27[i], "IDX_UNK27[%d]", i)
				}
			}
			for (int i = 0; i < 0x10; i++) {
				CMP(idx_state_vtxbuf_offset[i], "IDX_STATE_VTXBUF_OFFSET[%d]", i)
				CMP(idx_state_vtxbuf_format[i], "IDX_STATE_VTXBUF_FORMAT[%d]", i)
			}
			CMP(idx_state_a, "IDX_STATE_A")
			CMP(idx_state_b, "IDX_STATE_B")
			CMP(idx_state_c, "IDX_STATE_C")
			if (orig->chipset.card_type == 0x30) {
				CMP(idx_state_d, "IDX_STATE_D")
			}
			if (orig->chipset.card_type == 0x20) {
				CMP(fd_state_begin_pt_a, "FD_STATE_BEGIN_PT_A")
				CMP(fd_state_begin_pt_b, "FD_STATE_BEGIN_PT_B")
				CMP(fd_state_begin_patch_c, "FD_STATE_BEGIN_PATCH_C")
				CMP(fd_state_swatch, "FD_STATE_SWATCH")
				CMP(fd_state_unk10, "FD_STATE_UNK10")
				CMP(fd_state_unk14, "FD_STATE_UNK14")
				CMP(fd_state_unk18, "FD_STATE_UNK18")
				CMP(fd_state_unk1c, "FD_STATE_UNK1C")
				CMP(fd_state_unk20, "FD_STATE_UNK20")
				CMP(fd_state_unk24, "FD_STATE_UNK24")
				CMP(fd_state_unk28, "FD_STATE_UNK28")
				CMP(fd_state_unk2c, "FD_STATE_UNK2C")
				CMP(fd_state_unk30, "FD_STATE_UNK30")
				CMP(fd_state_unk34, "FD_STATE_UNK34")
				CMP(fd_state_begin_patch_d, "FD_STATE_BEGIN_PATCH_D")
			}
			for (int i = 0; i < 0x11; i++) {
				for (int j = 0; j < 4; j++) {
					CMP(vab[i][j], "VAB[%d][%d]", i, j)
				}
			}
			if (orig->chipset.card_type == 0x20) {
				for (int i = 0; i < 0x88; i++) {
					for (int j = 0; j < 4; j++) {
						CMP(xfpr[i][j], "XFPR[%d][%d]", i, j)
					}
				}
			} else {
				for (int i = 0; i < 0x118; i++) {
					for (int j = 0; j < 4; j++) {
						CMP(xfpr[i][j], "XFPR[%d][%d]", i, j)
					}
				}
				for (int i = 0; i < 2; i++) {
					for (int j = 0; j < 4; j++) {
						CMP(xfprunk1[i][j], "XFPRUNK1[%d][%d]", i, j)
					}
				}
				CMP(xfprunk2, "XFPRUNK2")
			}
		}
	}

	// DMA
	if (orig->chipset.card_type == 3) {
		CMP(dma_intr, "DMA_INTR")
		CMP_LIST(pgraph_dma_nv3_regs(orig->chipset));
	} else if (orig->chipset.card_type >= 4) {
		CMP_LIST(pgraph_dma_nv4_regs(orig->chipset));
	}
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}
