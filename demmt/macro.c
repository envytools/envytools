/*
 * Copyright (C) 2014 Marcin Ślusarz <marcin.slusarz@gmail.com>.
 * Copyright (C) 2010-2011 Marcin Kościelnicki <koriakin@0x04.net>
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
#define _GNU_SOURCE

#include "config.h"
#include "log.h"
#include "macro.h"
#include "object_state.h"
#include "object.h"
#include <stdlib.h>
#include "dis.h"

int macro_rt_verbose = 0;
int macro_rt = 1;
int macro_dis_enabled = 1;

#if 0
static void unsupp(const char *str)
{
	mmt_printf("ERROR: %s\n", str);
	demmt_abort();
}
#endif

static int reg1(uint32_t c)
{
	return (c >> 8) & 0x7;
}

static const char *reg1_str(uint32_t c)
{
	static char r[20];
	sprintf(r, "%s$r%d%s", colors->reg, reg1(c), colors->reset);
	return r;
}

static int reg2(uint32_t c)
{
	return (c >> 11) & 0x7;
}

static const char *reg2_str(uint32_t c)
{
	static char r[20];
	sprintf(r, "%s$r%d%s", colors->reg, reg2(c), colors->reset);
	return r;
}

static int reg3(uint32_t c)
{
	return (c >> 14) & 0x7;
}

static const char *reg3_str(uint32_t c)
{
	static char r[20];
	sprintf(r, "%s$r%d%s", colors->reg, reg3(c), colors->reset);
	return r;
}

static int imm(uint32_t c)
{
	return ((int)c) >> 14;
}

static const char *imm_str(uint32_t c)
{
	static char r[40];
	sprintf(r, "%s0x%x%s", colors->num, imm(c), colors->reset);
	return r;
}

static int srcpos(uint32_t c)
{
	return (c >> 17) & 0x1f;
}

static int size(uint32_t c)
{
	return (c >> 22) & 0x1f;
}

static int dstpos(uint32_t c)
{
	return (c >> 27) & 0x1f;
}

static int btarg(uint32_t c)
{
	return ((int)c) >> 14;
}

static const char *btarg_str(uint32_t c)
{
	static char r[40];
	sprintf(r, "%s%d%s", colors->btarg, btarg(c), colors->reset);
	return r;
}

static char *mcr_parm, *mcr_mov, *mcr_maddr, *mcr_send, *mcr_exit, *mcr_add,
			*mcr_adc, *mcr_sub, *mcr_sbb, *mcr_or, *mcr_xor, *mcr_extrshl,
			*mcr_extrinsrt, *mcr_parmsend, *mcr_maddrsend, *mcr_read, *mcr_bra,
			*mcr_braz, *mcr_branz, *mcr_and, *mcr_andn, *mcr_nand, *mcr_mthd,
			*mcr_incr, *mcr_nop, *mcr_annul;

static void init_macrodis()
{
	if (mcr_parm)
		return;
	if (asprintf(&mcr_parm,  "%sparm%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_mov,   "%smov%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_maddr, "%smaddr%s", colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_send,  "%ssend%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_exit,  "%sexit%s ", colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_add,   "%sadd%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_adc,   "%sadc%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_sub,   "%ssub%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_sbb,   "%ssbb%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_and,   "%sand%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_andn,  "%sandn%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_nand,  "%snand%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_or,    "%sor%s",    colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_xor,   "%sxor%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_read,  "%sread%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_bra,   "%sbra%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_braz,  "%sbraz%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_branz, "%sbranz%s", colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_mthd,  "%smthd%s",  colors->reg,   colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_incr,  "%sincr%s",  colors->reg,   colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_nop,   "%snop%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_annul, " %sannul%s",colors->mod,   colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_extrshl,   "%sextrshl%s",   colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_extrinsrt, "%sextrinsrt%s", colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_parmsend,  "%sparmsend%s",  colors->rname, colors->reset) < 0) demmt_abort();
	if (asprintf(&mcr_maddrsend, "%smaddrsend%s", colors->rname, colors->reset) < 0) demmt_abort();
}

static void fini_str(char **c)
{
	free(*c);
	*c = NULL;
}

void fini_macrodis()
{
	fini_str(&mcr_parm);
	fini_str(&mcr_mov);
	fini_str(&mcr_maddr);
	fini_str(&mcr_send);
	fini_str(&mcr_exit);
	fini_str(&mcr_add);
	fini_str(&mcr_adc);
	fini_str(&mcr_sub);
	fini_str(&mcr_sbb);
	fini_str(&mcr_and);
	fini_str(&mcr_andn);
	fini_str(&mcr_nand);
	fini_str(&mcr_or);
	fini_str(&mcr_xor);
	fini_str(&mcr_read);
	fini_str(&mcr_bra);
	fini_str(&mcr_braz);
	fini_str(&mcr_branz);
	fini_str(&mcr_mthd);
	fini_str(&mcr_incr);
	fini_str(&mcr_nop);
	fini_str(&mcr_annul);
	fini_str(&mcr_extrshl);
	fini_str(&mcr_extrinsrt);
	fini_str(&mcr_parmsend);
	fini_str(&mcr_maddrsend);
}

static char outs[1000];
static char outs2[1000];
static char dec_obj[1000], dec_mthd[1000], dec_val[1000];

static inline void decode_maddr(uint32_t maddr, uint32_t *mthd, uint32_t *incr)
{
	*mthd = (maddr & 0xfff) << 2;
	if (incr)
		*incr = (maddr >> 12) & 0x3f;
}

static int macro_dis_dst(char *out, uint32_t c)
{
	int expects_maddr = 0;
	if ((c & 0x00000070) == 0x00000000)
	{
		// { 0x00000000, 0x00000070, N("parm"), REG1, N("ign") }
		sprintf(out, "%s %s ign", mcr_parm, reg1_str(c));
	}
	else if ((c & 0x00000070) == 0x00000010)
	{
		// { 0x00000010, 0x00000070, N("mov"), REG1 }
		sprintf(out, "%s %s", mcr_mov, reg1_str(c));
	}
	else if ((c & 0x00000770) == 0x00000020)
	{
		// { 0x00000020, 0x00000770, N("maddr") }
		sprintf(out, "%s", mcr_maddr);
		expects_maddr = 1;
	}
	else if ((c & 0x00000070) == 0x00000020)
	{
		// { 0x00000020, 0x00000070, N("maddr"), REG1 }
		sprintf(out, "%s %s", mcr_maddr, reg1_str(c));
		expects_maddr = 1;
	}
	else if ((c & 0x00000070) == 0x00000030)
	{
		// { 0x00000030, 0x00000070, N("parm"), REG1, N("send") }
		sprintf(out, "%s %s %s", mcr_parm, reg1_str(c), mcr_send);
	}
	else if ((c & 0x00000770) == 0x00000040)
	{
		// { 0x00000040, 0x00000770, N("send") }
		sprintf(out, "%s", mcr_send);
	}
	else if ((c & 0x00000070) == 0x00000040)
	{
		// { 0x00000040, 0x00000070, N("send"), REG1 }
		sprintf(out, "%s %s", mcr_send, reg1_str(c));
	}
	else if ((c & 0x00000070) == 0x00000050)
	{
		// { 0x00000050, 0x00000070, N("parm"), REG1, N("maddr") }
		sprintf(out, "%s %s %s", mcr_parm, reg1_str(c), mcr_maddr);
		return 1;
	}
	else if ((c & 0x00000770) == 0x00000060)
	{
		// { 0x00000060, 0x00000770, N("parmsend"), N("maddr") }
		sprintf(out, "%s %s", mcr_parmsend, mcr_maddr);
		expects_maddr = 1;
	}
	else if ((c & 0x00000070) == 0x00000060)
	{
		// { 0x00000060, 0x00000070, N("parmsend"), N("maddr"), REG1 }
		sprintf(out, "%s %s %s", mcr_parmsend, mcr_maddr, reg1_str(c));
		expects_maddr = 1;
	}
	else if ((c & 0x00000770) == 0x00000070)
	{
		// { 0x00000070, 0x00000770, N("maddrsend") }
		sprintf(out, "%s", mcr_maddrsend);
		expects_maddr = 1;
	}
	else if ((c & 0x00000070) == 0x00000070)
	{
		// { 0x00000070, 0x00000070, N("maddrsend"), REG1 }
		sprintf(out, "%s %s", mcr_maddrsend, reg1_str(c));
		expects_maddr = 1;
	}
	else
		sprintf(out, "???");

	return expects_maddr;
}

static void macro_dis(uint32_t *code, uint32_t words, struct obj *obj)
{
	uint32_t i;
	init_macrodis();

	for (i = 0; i < words; ++i)
	{
		uint32_t c = code[i];

		mmt_printf("MC: 0x%08x   ", c);
		if (c & 0x80)
			mmt_printf("%s", mcr_exit);

		if ((c & 0x003e0007) == 0x00000000)
		{
			// { 0x00000000, 0x003e0007, T(dst), SESTART, N("add"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_add, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00020000)
		{
			// { 0x00020000, 0x003e0007, T(dst), SESTART, N("adc"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_adc, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00040000)
		{
			// { 0x00040000, 0x003e0007, T(dst), SESTART, N("sub"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_sub, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00060000)
		{
			// { 0x00060000, 0x003e0007, T(dst), SESTART, N("sbb"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_sbb, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00100000)
		{
			// { 0x00100000, 0x003e0007, T(dst), SESTART, N("xor"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_xor, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00120000)
		{
			// { 0x00120000, 0x003e0007, T(dst), SESTART, N("or"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_or, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00140000)
		{
			// { 0x00140000, 0x003e0007, T(dst), SESTART, N("and"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_and, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00160000)
		{
			// { 0x00160000, 0x003e0007, T(dst), SESTART, N("andn"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_andn, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0x003e0007) == 0x00180000)
		{
			// { 0x00180000, 0x003e0007, T(dst), SESTART, N("nand"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_nand, reg2_str(c), reg3_str(c));
		}
		else if ((c & 0xffffffff) == 0x00000011)
		{
			// { 0x00000011, 0xffffffff, N("nop") }
			mmt_printf("%s\n", mcr_nop);
		}
		else if ((c & 0xffffffff) == 0x00000091)
		{
			// { 0x00000091, 0xffffffff }
			mmt_printf("ffs%s\n", "");
		}
		else if ((c & 0xfffff87f) == 0x00000001)
		{
			// { 0x00000001, 0xfffff87f, N("parm"), REG1 }
			mmt_printf("%s %s\n", mcr_parm, reg1_str(c));
		}
		else if ((c & 0x00003807) == 0x00000001)
		{
			// { 0x00000001, 0x00003807, T(dst), MIMM }
			int expects_maddr = macro_dis_dst(outs, c);
			if (expects_maddr)
			{
				uint32_t mthd;
				decode_maddr(imm(c), &mthd, NULL);
				decode_method_raw(mthd, 0, obj, dec_obj, dec_mthd, NULL);

				mmt_printf("%s %s [%s.%s]\n", outs, imm_str(c), dec_obj, dec_mthd);
			}
			else
				mmt_printf("%s %s\n", outs, imm_str(c));
		}
		else if ((c & 0xffffc007) == 0x00000001)
		{
			// { 0x00000001, 0xffffc007, T(dst), REG2 }
			macro_dis_dst(outs, c);
			mmt_printf("%s %s\n", outs, reg2_str(c));
		}
		else if ((c & 0x00000007) == 0x00000001)
		{
			// { 0x00000001, 0x00000007, T(dst), SESTART, N("add"), REG2, MIMM, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s)\n", outs, mcr_add, reg2_str(c), imm_str(c));
		}
		else if ((c & 0x00000007) == 0x00000002)
		{
			// { 0x00000002, 0x00000007, T(dst), SESTART, N("extrinsrt"), REG2, REG3, BFSRCPOS, BFSZ, BFDSTPOS, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s %s0x%x 0x%x 0x%x%s)\n", outs,
					mcr_extrinsrt, reg2_str(c), reg3_str(c), colors->num,
					srcpos(c), size(c), dstpos(c), colors->reset);
		}
		else if ((c & 0x00000007) == 0x00000003)
		{
			// { 0x00000003, 0x00000007, T(dst), SESTART, N("extrshl"), REG3, REG2, BFSZ, BFDSTPOS, SEEND },
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s %s0x%x 0x%x%s)\n", outs, mcr_extrshl,
					reg3_str(c), reg2_str(c), colors->num, size(c), dstpos(c),
					colors->reset);
		}
		else if ((c & 0x00000007) == 0x00000004)
		{
			// { 0x00000004, 0x00000007, T(dst), SESTART, N("extrshl"), REG3, BFSRCPOS, BFSZ, REG2, SEEND }
			macro_dis_dst(outs, c);
			mmt_printf("%s (%s %s %s0x%x 0x%x%s %s)\n", outs, mcr_extrshl,
					reg3_str(c), colors->num, srcpos(c), size(c), colors->reset,
					reg2_str(c));
		}
		else if ((c & 0x00003877) == 0x00000015)
		{
			// { 0x00000015, 0x00003877, N("read"), REG1, MIMM }
			uint32_t mthd;
			decode_maddr(imm(c), &mthd, NULL);
			decode_method_raw(mthd, 0, obj, dec_obj, dec_mthd, NULL);

			mmt_printf("%s %s %s [%s.%s]\n", mcr_read, reg1_str(c), imm_str(c),
					dec_obj, dec_mthd);
		}
		else if ((c & 0x00000077) == 0x00000015)
		{
			// { 0x00000015, 0x00000077, N("read"), REG1, SESTART, N("add"), REG2, MIMM, SEEND }
			mmt_printf("%s %s (%s %s %s)\n", mcr_read, reg1_str(c), mcr_add,
					reg2_str(c), imm_str(c));
		}
		else if ((c & 0x00003817) == 0x00000007)
		{
			// { 0x00000007, 0x00003817, N("bra"), T(annul), BTARG }
			mmt_printf("%s%s %s\n", mcr_bra, (c & 0x20) ? mcr_annul : "",
					btarg_str(c));
		}
		else if ((c & 0x00000017) == 0x00000007)
		{
			// { 0x00000007, 0x00000017, N("braz"), T(annul), REG2, BTARG }
			mmt_printf("%s%s %s %s\n", mcr_braz, (c & 0x20) ? mcr_annul : "",
					reg2_str(c), btarg_str(c));
		}
		else if ((c & 0x00000017) == 0x00000017)
		{
			// { 0x00000017, 0x00000017, N("branz"), T(annul), REG2, BTARG }
			mmt_printf("%s%s %s %s\n", mcr_branz, (c & 0x20) ? mcr_annul : "",
					reg2_str(c), btarg_str(c));
		}
		else
		{
			// { 0, 0, T(dst), OOPS, REG2 }
			macro_dis_dst(outs, c);
			mmt_printf("%s (??? %s)\n", outs, reg2_str(c));
		}
	}
}

static void register_method_call(struct macro_interpreter_state *istate, uint32_t res)
{
	struct pushbuf_decode_state pstate = { 0 };
	struct obj *obj = istate->obj;

	if (istate->mthd < OBJECT_SIZE)
		obj->data[istate->mthd / 4] = res;
	else
		mmt_printf("method 0x%x >= 0x%x\n", istate->mthd, OBJECT_SIZE);

	if (macro_rt_verbose)
		return;

	mmt_printf("PM: 0x%08x   %s.%s = %s", res, dec_obj, dec_mthd, dec_val);

	pstate.mthd = istate->mthd;
	pstate.mthd_data = res;
	pstate.fifo = istate->device;

	if (obj->decoder && obj->decoder->decode_terse)
		obj->decoder->decode_terse(obj->gpu_object, &pstate);
	mmt_printf("%s\n", "");

	if (obj->decoder && obj->decoder->decode_verbose)
		obj->decoder->decode_verbose(obj->gpu_object, &pstate);
}

static int macro_sim_dst(char *out, uint32_t c, uint32_t res,
		struct macro_interpreter_state *istate)
{
	uint32_t *regs = istate->regs;
	uint32_t *mthd = &istate->mthd;
	uint32_t *incr = &istate->incr;
	struct obj *obj = istate->obj;

	if ((c & 0x00000070) == 0x00000000)
	{
		// { 0x00000000, 0x00000070, N("parm"), REG1, N("ign") }
		// ignore result, fetch param to REG1
		if (!istate->macro_param)
			return 1;
		sprintf(outs,
				"%s := %s0x%x%s", // $rX := 0x...
				reg1_str(c), colors->num, *istate->macro_param, colors->reset);

		regs[reg1(c)] = *istate->macro_param;
		istate->macro_param = NULL;
	}
	else if ((c & 0x00000070) == 0x00000010)
	{
		// { 0x00000010, 0x00000070, N("mov"), REG1 }
		// store result to REG1
		sprintf(out,
				"%s := %s0x%x%s", // $rX := 0x...
				reg1_str(c), colors->num, res, colors->reset);

		regs[reg1(c)] = res;
	}
	else if ((c & 0x00000770) == 0x00000020)
	{
		// { 0x00000020, 0x00000770, N("maddr") }
		decode_maddr(res, mthd, incr);

		decode_method_raw(*mthd, 0, obj, dec_obj, dec_mthd, NULL);
		sprintf(out,
				"%s := %s0x%x%s [%s.%s], " // mthd := 0x.... [NVXX_3D....]
				"%s := %s%d%s",            // incr := ...
				mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset);
	}
	else if ((c & 0x00000070) == 0x00000020)
	{
		// { 0x00000020, 0x00000070, N("maddr"), REG1 }
		// use result as maddr and store it to REG1
		decode_maddr(res, mthd, incr);

		decode_method_raw(*mthd, 0, obj, dec_obj, dec_mthd, NULL);
		sprintf(out,
				"%s := %s := %s0x%x%s [%s.%s], " // $rX := mthd := 0x... [NVXX_3D....]
				"%s := %s%d%s",                  // incr := ...
				reg1_str(c), mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset);

		regs[reg1(c)] = res;
	}
	else if ((c & 0x00000070) == 0x00000030)
	{
		// { 0x00000030, 0x00000070, N("parm"), REG1, N("send") }
		// send result, then fetch param to REG1
		if (!istate->macro_param)
			return 1;

		decode_method_raw(*mthd, res, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, res);

		sprintf(out,
				"%s.%s := %s0x%x%s [%s], " // NVXX_3D.... := 0x... [desc_val]
				"%s := %s0x%x%s",          // $rX := 0x...
				dec_obj, dec_mthd, colors->num, res, colors->reset, dec_val,
				reg1_str(c), colors->num, *istate->macro_param, colors->reset);

		*mthd += *incr * 4;
		regs[reg1(c)] = *istate->macro_param;
		istate->macro_param = NULL;
	}
	else if ((c & 0x00000770) == 0x00000040)
	{
		// { 0x00000040, 0x00000770, N("send") }
		decode_method_raw(*mthd, res, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, res);

		sprintf(out,
				"%s.%s := %s0x%x%s [%s]", // NVXX_3D.... := 0x... [desc_val]
				dec_obj, dec_mthd, colors->num, res, colors->reset, dec_val);

		*mthd += *incr * 4;
	}
	else if ((c & 0x00000070) == 0x00000040)
	{
		// { 0x00000040, 0x00000070, N("send"), REG1 }
		// send result and store it to REG1
		decode_method_raw(*mthd, res, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, res);

		sprintf(out,
				"%s.%s := %s0x%x%s [%s], " // NVXX_3D.... := 0x... [desc_val]
				"%s := %s0x%x%s",          // $rX := 0x...
				dec_obj, dec_mthd, colors->num, res, colors->reset, dec_val,
				reg1_str(c), colors->num, res, colors->reset);

		*mthd += *incr * 4;
		regs[reg1(c)] = res;
	}
	else if ((c & 0x00000070) == 0x00000050)
	{
		// { 0x00000050, 0x00000070, N("parm"), REG1, N("maddr") }
		// use result as maddr, then fetch param to REG1
		if (!istate->macro_param)
			return 1;

		decode_maddr(res, mthd, incr);

		decode_method_raw(*mthd, 0, obj, dec_obj, dec_mthd, NULL);

		sprintf(out,
				"%s := %s0x%x%s [%s.%s], " // mthd := 0x... [NVXX_3D....]
				"%s := %s%d%s, "           // incr := ...
				"%s := %s0x%x%s",          // $rX := 0x...
				mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset,
				reg1_str(c), colors->num, *istate->macro_param, colors->reset);

		regs[reg1(c)] = *istate->macro_param;
		istate->macro_param = NULL;
	}
	else if ((c & 0x00000770) == 0x00000060)
	{
		// { 0x00000060, 0x00000770, N("parmsend"), N("maddr") }
		// use result as maddr, then fetch param and send it.
		if (!istate->macro_param)
			return 1;

		decode_maddr(res, mthd, incr);

		decode_method_raw(*mthd, *istate->macro_param, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, *istate->macro_param);

		sprintf(out,
				"%s := %s0x%x%s [%s.%s], " // mthd := 0x... [NVXX_3D....]
				"%s := %s%d%s, "           // incr := ...
				"%s.%s := %s0x%x%s [%s]",  // NVXX_3D.... := 0x... [desc_val]
				mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset,
				dec_obj, dec_mthd, colors->num, *istate->macro_param, colors->reset, dec_val);

		*mthd += *incr * 4;
		istate->macro_param = NULL;
	}
	else if ((c & 0x00000070) == 0x00000060)
	{
		// { 0x00000060, 0x00000070, N("parmsend"), N("maddr"), REG1 }
		// use result as maddr and store it to REG1, then fetch param and send it.
		if (!istate->macro_param)
			return 1;

		decode_maddr(res, mthd, incr);

		decode_method_raw(*mthd, *istate->macro_param, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, *istate->macro_param);

		sprintf(out,
				"%s := %s0x%x%s, "         // rX := 0x...
				"%s := %s0x%x%s [%s.%s], " // mthd := 0x... [NVXX_3D....]
				"%s := %s%d%s, "           // incr := 0x...
				"%s.%s := %s0x%x%s [%s]",  // NVXX_3D.... := 0x... [desc_val]
				reg1_str(c), colors->num, res, colors->reset,
				mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset,
				dec_obj, dec_mthd, colors->num, *istate->macro_param, colors->reset, dec_val);

		*mthd += *incr * 4;
		regs[reg1(c)] = res;
		istate->macro_param = NULL;
	}
	else if ((c & 0x00000770) == 0x00000070)
	{
		// { 0x00000070, 0x00000770, N("maddrsend") }
		// use result as maddr, then send bits 12-17 of result
		decode_maddr(res, mthd, NULL);

		uint32_t mthd_data = (res >> 12) & 0x3f;

		decode_method_raw(*mthd, mthd_data, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, mthd_data);

		sprintf(out,
				"%s := %s0x%x%s [%s.%s], " // mthd := 0x... [NVXX_3D....]
				"%s := %s%d%s, "           // incr := ...
				"%s.%s := %s0x%x%s [%s]",  // NVXX_3D.... := 0x... [desc_val]
				mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset,
				dec_obj, dec_mthd, colors->num, mthd_data, colors->reset, dec_val);

		*mthd += *incr * 4;
	}
	else if ((c & 0x00000070) == 0x00000070)
	{
		// { 0x00000070, 0x00000070, N("maddrsend"), REG1 }
		// use result as maddr, then send bits 12-17 of result, then store result to REG1
		decode_maddr(res, mthd, NULL);

		uint32_t mthd_data = (res >> 12) & 0x3f;

		decode_method_raw(*mthd, mthd_data, obj, dec_obj, dec_mthd, dec_val);
		register_method_call(istate, mthd_data);

		sprintf(out,
				"%s := %s0x%x%s [%s.%s], " // mthd := 0x... [NVXX_3D....]
				"%s := %s%d%s, "           // incr := ...
				"%s.%s := %s0x%x%s [%s], " // NVXX_3D.... := 0x... [desc_val]
				"%s := %s0x%x%s",          // $rX := 0x...
				mcr_mthd, colors->num, *mthd, colors->reset, dec_obj, dec_mthd,
				mcr_incr, colors->num, *incr, colors->reset,
				dec_obj, dec_mthd, colors->num, mthd_data, colors->reset, dec_val,
				reg1_str(c), colors->num, res, colors->reset);

		regs[reg1(c)] = res;
		*mthd += *incr * 4;
	}
	else
		sprintf(out, "???");

	return 0;
}

#define ALGN 65

static void print_aligned(char *str)
{
	int i = 0, clen = strlen(str), vislen = 0;
	for (i = 0; i < clen; ++i)
	{
		// skip all color escape codes
		if (str[i] == '\x1b')
		{
			while (str[i] != 'm')
				++i;
			++i;
		}
		if (str[i])
			++vislen;
	}
	if (i == clen + 1)
		--i;

	while (vislen++ < ALGN)
		str[i++] = ' ';
	str[i] = 0;

	mmt_printf("%s", str);
}

#define MAX_BACK_JUMPS 100

static void macro_sim(struct macro_interpreter_state *istate)
{
	char pfx[100] = "";
	uint32_t *regs = istate->regs;
	struct obj *obj = istate->obj;

	init_macrodis();

	while (!istate->aborted)
	{
		uint32_t c = istate->code[istate->pc];
		sprintf(pfx, "MC: 0x%08x   ", c);
		if (c & 0x80)
		{
			strcat(pfx, mcr_exit);
			istate->exit_when_0 = 2;
		}

		if (istate->pc < istate->lastpc && istate->backward_jumps++ > MAX_BACK_JUMPS)
		{
			mmt_error("more than %d backward jumps, aborting macro simulation\n", MAX_BACK_JUMPS);
			istate->aborted = 1;
			return;
		}
		istate->lastpc = istate->pc;

		if ((c & 0x003e0007) == 0x00000000)
		{
			// { 0x00000000, 0x003e0007, T(dst), SESTART, N("add"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_add, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] + regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00020000)
		{
			// { 0x00020000, 0x003e0007, T(dst), SESTART, N("adc"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_adc, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] + regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s (TODO: CF)\n", outs); // TODO: carry flag
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00040000)
		{
			// { 0x00040000, 0x003e0007, T(dst), SESTART, N("sub"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_sub, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] - regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00060000)
		{
			// { 0x00060000, 0x003e0007, T(dst), SESTART, N("sbb"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_sbb, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] - regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s (TODO: CF)\n", outs); // TODO: carry flag
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00100000)
		{
			// { 0x00100000, 0x003e0007, T(dst), SESTART, N("xor"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_xor, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] ^ regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00120000)
		{
			// { 0x00120000, 0x003e0007, T(dst), SESTART, N("or"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_or, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] | regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00140000)
		{
			// { 0x00140000, 0x003e0007, T(dst), SESTART, N("and"), REG2, REG3, SEEND }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_and, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] & regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00160000)
		{
			// { 0x00160000, 0x003e0007, T(dst), SESTART, N("andn"), REG2, REG3, SEEND }
			// REG2 & ~REG3
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_andn, reg2_str(c),
					reg3_str(c));

			uint32_t res = regs[reg2(c)] & ~regs[reg3(c)];
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x003e0007) == 0x00180000)
		{
			// { 0x00180000, 0x003e0007, T(dst), SESTART, N("nand"), REG2, REG3, SEEND }
			// ~(REG2 & REG3)
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_nand, reg2_str(c),
					reg3_str(c));

			uint32_t res = ~(regs[reg2(c)] & regs[reg3(c)]);
			if (macro_sim_dst(outs, c, res, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0xffffffff) == 0x00000011)
		{
			// { 0x00000011, 0xffffffff, N("nop") }
			sprintf(outs, "%s%s", pfx, mcr_nop);
			if (macro_rt_verbose)
			{
				print_aligned(outs);
				mmt_printf(" | %s\n", mcr_nop);
			}
			++istate->pc;
		}
		else if ((c & 0xffffffff) == 0x00000091)
		{
			// { 0x00000091, 0xffffffff }
			// [just exit]
			sprintf(outs, "%sffs", pfx);
			if (macro_rt_verbose)
			{
				print_aligned(outs);
				mmt_printf(" | %s\n", "");
			}
			++istate->pc;
		}
		else if ((c & 0xfffff87f) == 0x00000001)
		{
			// { 0x00000001, 0xfffff87f, N("parm"), REG1 }
			if (istate->macro_param == NULL)
				return;
			sprintf(outs, "%s%s %s", pfx, mcr_parm, reg1_str(c));
			if (macro_rt_verbose)
			{
				print_aligned(outs);
				mmt_printf(" | %s := %s0x%x%s\n", reg1_str(c), colors->num,
						*istate->macro_param, colors->reset);
			}
			regs[reg1(c)] = *istate->macro_param;
			istate->macro_param = NULL;
			++istate->pc;
		}
		else if ((c & 0x00003807) == 0x00000001)
		{
			// { 0x00000001, 0x00003807, T(dst), MIMM }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s %s", pfx, outs, imm_str(c));

			uint32_t val = imm(c);
			if (macro_sim_dst(outs, c, val, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0xffffc007) == 0x00000001)
		{
			// { 0x00000001, 0xffffc007, T(dst), REG2 }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s %s", pfx, outs, reg2_str(c));

			uint32_t val = regs[reg2(c)];
			if (macro_sim_dst(outs, c, val, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x00000007) == 0x00000001)
		{
			// { 0x00000001, 0x00000007, T(dst), SESTART, N("add"), REG2, MIMM, SEEND },
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s)", pfx, outs, mcr_add, reg2_str(c),
					imm_str(c));

			uint32_t val = regs[reg2(c)] + imm(c);
			if (macro_sim_dst(outs, c, val, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x00000007) == 0x00000002)
		{
			// take REG2, replace BFSZ bits starting at BFDSTPOS with BFSZ bits starting at BFSRCPOS in REG3.
			// { 0x00000002, 0x00000007, T(dst), SESTART, N("extrinsrt"), REG2, REG3, BFSRCPOS, BFSZ, BFDSTPOS, SEEND }

			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s %s0x%x 0x%x 0x%x%s)", pfx, outs,
					mcr_extrinsrt, reg2_str(c), reg3_str(c), colors->num,
					srcpos(c), size(c), dstpos(c), colors->reset);

			uint32_t v1 = regs[reg2(c)];
			uint32_t v2 = regs[reg3(c)];
			uint32_t pos1 = dstpos(c);
			uint32_t pos2 = srcpos(c);
			uint32_t sz = size(c);

			uint32_t mask1 = ((1 << (sz + 1)) - 1) << pos1;
			uint32_t mask2 = ((1 << (sz + 1)) - 1) << pos2;

			uint32_t v = (v1 & ~mask1) | (((v2 & mask2) >> pos2) << pos1);

			if (macro_sim_dst(outs, c, v, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x00000007) == 0x00000003)
		{
			// { 0x00000003, 0x00000007, T(dst), SESTART, N("extrshl"), REG3, REG2, BFSZ, BFDSTPOS, SEEND },
			// take BFSZ bits starting at REG2 in REG3, shift left by BFDSTPOS
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s %s0x%x 0x%x%s)", pfx, outs,
					mcr_extrshl, reg3_str(c), reg2_str(c), colors->num,
					size(c), dstpos(c), colors->reset);

			uint32_t v2 = regs[reg2(c)];
			uint32_t v3 = regs[reg3(c)];
			uint32_t dpos = dstpos(c);
			uint32_t sz = size(c);

			uint32_t mask = ((1 << (sz + 1)) - 1) << v2;

			uint32_t v = ((v3 & mask) >> v2) << dpos;

			if (macro_sim_dst(outs, c, v, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x00000007) == 0x00000004)
		{
			// { 0x00000004, 0x00000007, T(dst), SESTART, N("extrshl"), REG3, BFSRCPOS, BFSZ, REG2, SEEND }
			// take BFSZ bits starting at BFSRCPOS in REG3, shift left by REG2
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s (%s %s %s0x%x 0x%x%s %s)", pfx, outs,
					mcr_extrshl, reg3_str(c), colors->num, srcpos(c),
					size(c), colors->reset, reg2_str(c));

			uint32_t v2 = regs[reg2(c)];
			uint32_t v3 = regs[reg3(c)];
			uint32_t spos = srcpos(c);
			uint32_t sz = size(c);

			uint32_t mask = ((1 << (sz + 1)) - 1) << spos;

			uint32_t v = ((v3 & mask) >> spos) << v2;

			if (macro_sim_dst(outs, c, v, istate))
				return;

			if (macro_rt_verbose)
			{
				print_aligned(outs2);
				mmt_printf(" | %s\n", outs);
			}
			++istate->pc;
		}
		else if ((c & 0x00003877) == 0x00000015)
		{
			// { 0x00000015, 0x00003877, N("read"), REG1, MIMM }
			sprintf(outs, "%s%s %s %s", pfx, mcr_read, reg1_str(c), imm_str(c));
			if (macro_rt_verbose)
				print_aligned(outs);

			uint32_t mthd = (imm(c) & 0xfff) << 2;
			uint32_t mthd_data =  obj->data[mthd / 4];
			regs[reg1(c)] = mthd_data;

			decode_method_raw(mthd, mthd_data, obj, dec_obj, dec_mthd, dec_val);

			sprintf(outs, "%s := %s.%s = %s0x%x%s [%s]", reg1_str(c), dec_obj,
					dec_mthd, colors->num, mthd_data, colors->reset, dec_val);
			if (macro_rt_verbose)
				mmt_printf(" | %s\n", outs);

			++istate->pc;
		}
		else if ((c & 0x00000077) == 0x00000015)
		{
			// { 0x00000015, 0x00000077, N("read"), REG1, SESTART, N("add"), REG2, MIMM, SEEND }
			sprintf(outs, "%s%s %s (%s %s %s)", pfx, mcr_read, reg1_str(c),
					mcr_add, reg2_str(c), imm_str(c));
			if (macro_rt_verbose)
				print_aligned(outs);

			uint32_t val = regs[reg2(c)] + imm(c);

			uint32_t mthd = (val & 0xfff) << 2;
			uint32_t mthd_data =  obj->data[mthd / 4];
			regs[reg1(c)] = mthd_data;

			decode_method_raw(mthd, mthd_data, obj, dec_obj, dec_mthd, dec_val);

			sprintf(outs, "%s := %s.%s = %s0x%x%s [%s]", reg1_str(c), dec_obj,
					dec_mthd, colors->num, mthd_data, colors->reset, dec_val);
			if (macro_rt_verbose)
				mmt_printf(" | %s\n", outs);

			++istate->pc;
		}
		else if ((c & 0x00003817) == 0x00000007)
		{
			// { 0x00000007, 0x00003817, N("bra"), T(annul), BTARG }
			sprintf(outs, "%s%s%s %s", pfx, mcr_bra, (c & 0x20) ? mcr_annul : "",
					btarg_str(c));

			if (macro_rt_verbose)
				print_aligned(outs);
			if (c & 0x20)
			{
				sprintf(outs, "%sPC +=%s %s", colors->mod, colors->reset,
						btarg_str(c));
				if (macro_rt_verbose)
					mmt_printf(" | %s\n", outs);
				istate->pc += btarg(c);
			}
			else
			{
				sprintf(outs, "%sPC +=%s %s %s(delayed)%s", colors->mod,
						colors->reset, btarg_str(c), colors->comm, colors->reset);
				if (macro_rt_verbose)
					mmt_printf(" | %s\n", outs);
				istate->delayed_pc = istate->pc + btarg(c);
				++istate->pc;
				continue;
			}
		}
		else if ((c & 0x00000017) == 0x00000007)
		{
			// { 0x00000007, 0x00000017, N("braz"), T(annul), REG2, BTARG }
			sprintf(outs, "%s%s%s %s %s", pfx, mcr_braz, (c & 0x20) ? mcr_annul : "",
					reg2_str(c), btarg_str(c));
			if (macro_rt_verbose)
				print_aligned(outs);

			if (regs[reg2(c)] == 0)
			{
				if (c & 0x20)
				{
					sprintf(outs, "%sPC +=%s %s", colors->mod, colors->reset,
							btarg_str(c));
					if (macro_rt_verbose)
						mmt_printf(" | %s\n", outs);
					istate->pc += btarg(c);
				}
				else
				{
					sprintf(outs, "%sPC +=%s %s %s(delayed)%s", colors->mod,
							colors->reset, btarg_str(c), colors->comm,
							colors->reset);
					if (macro_rt_verbose)
						mmt_printf(" | %s\n", outs);
					istate->delayed_pc = istate->pc + btarg(c);
					++istate->pc;
					continue;
				}
			}
			else
			{
				sprintf(outs, "%s", mcr_nop);
				if (macro_rt_verbose)
					mmt_printf(" | %s\n", outs);
				++istate->pc;
				if (c & 0x80) // exit cancelled
					istate->exit_when_0 = 0xffffffff;
			}
		}
		else if ((c & 0x00000017) == 0x00000017)
		{
			// { 0x00000017, 0x00000017, N("branz"), T(annul), REG2, BTARG }
			sprintf(outs, "%s%s%s %s %s", pfx, mcr_branz, (c & 0x20) ? mcr_annul : "",
					reg2_str(c), btarg_str(c));
			if (macro_rt_verbose)
				print_aligned(outs);

			if (regs[reg2(c)] != 0)
			{
				if (c & 0x20)
				{
					sprintf(outs, "%sPC +=%s %s", colors->mod, colors->reset,
							btarg_str(c));
					if (macro_rt_verbose)
						mmt_printf(" | %s\n", outs);
					istate->pc += btarg(c);
				}
				else
				{
					sprintf(outs, "%sPC +=%s %s %s(delayed)%s", colors->mod,
							colors->reset, btarg_str(c), colors->comm,
							colors->reset);
					if (macro_rt_verbose)
						mmt_printf(" | %s\n", outs);
					istate->delayed_pc = istate->pc + btarg(c);
					++istate->pc;
					continue;
				}
			}
			else
			{
				sprintf(outs, "%s", mcr_nop);
				if (macro_rt_verbose)
					mmt_printf(" | %s\n", outs);
				++istate->pc;
				if (c & 0x80) // exit cancelled
					istate->exit_when_0 = 0xffffffff;
			}
		}
		else
		{
			// { 0, 0, T(dst), OOPS, REG2 }
			macro_dis_dst(outs, c);
			sprintf(outs2, "%s%s ???", pfx, outs);

			if (macro_rt_verbose || 1)
			{
				print_aligned(outs2);
				mmt_printf(" | ???, aborting%s\n", "");
			}
			++istate->pc;
			istate->aborted = 1;
		}

		if (istate->delayed_pc != 0xffffffff)
		{
			istate->pc = istate->delayed_pc;
			istate->delayed_pc = 0xffffffff;
		}

		if (istate->exit_when_0 != 0xffffffff)
		{
			if (--istate->exit_when_0 == 0)
				break;
		}
	}
}

int decode_macro(struct pushbuf_decode_state *pstate, struct macro_state *macro)
{
	int mthd = pstate->mthd;
	uint32_t data = pstate->mthd_data;

	if (mthd == 0x0114) // GRAPH.MACRO_CODE_POS
	{
		if (macro->code == NULL)
			macro->code = calloc(0x2000, 1);
		macro->last_code_pos = data * 4;
		macro->cur_code_pos = data * 4;
	}
	else if (mthd == 0x0118) // GRAPH.MACRO_CODE_DATA
	{
		if (macro->cur_code_pos >= 0x2000)
			mmt_log("not enough space for more macro code, truncating%s\n", "");
		else
		{
			macro->code[macro->cur_code_pos / 4] = data;
			macro->cur_code_pos += 4;
			if (pstate->size == 0)
			{
				int i;
				for (i = 0; i < 0x80; ++i)
					if (macro->entries[i].start == macro->last_code_pos)
					{
						macro->entries[i].words = (macro->cur_code_pos - macro->entries[i].start) / 4;
						break;
					}

				if (!isa_macro)
					isa_macro = ed_getisa("macro");
				if (MMT_DEBUG && macro_dis_enabled)
				{
					struct varinfo *var = varinfo_new(isa_macro->vardata);

					envydis(isa_macro, stdout, (void *)(macro->code + macro->last_code_pos / 4), 0,
							(macro->cur_code_pos - macro->last_code_pos) / 4,
							var, 0, NULL, 0, colors);
					varinfo_del(var);
				}

				if (macro_dis_enabled)
					macro_dis(macro->code + macro->last_code_pos / 4,
							(macro->cur_code_pos - macro->last_code_pos) / 4,
							current_subchan_object(pstate));
			}
		}
	}
	else if (mthd == 0x011c) // GRAPH.MACRO_ENTRY_POS
		macro->last_entry_pos = data;
	else if (mthd == 0x0120) // GRAPH.MACRO_ENTRY_DATA
	{
		if (macro->last_entry_pos >= 0x80)
			mmt_log("macro position (0x%x) over limit\n", macro->last_entry_pos);
		else
		{
			macro->entries[macro->last_entry_pos].start = data * 4;
			mmt_debug("binding entry at 0x%x to position 0x%x\n", data,
					macro->last_entry_pos);
		}
	}
	else if (mthd >= 0x3800 && mthd < 0x3800 + 0x80 * 8) // GRAPH.MACRO / GRAPH.MACRO_PARAM
	{
		int macro_offset = mthd - 0x3800;
		int macro_idx = macro_offset / 8;

		mmt_debug("macro_offset: 0x%x, macro_idx: 0x%x\n", macro_offset, macro_idx);

		if ((macro_offset & 0x7) == 0)
		{
			mmt_debug("MACRO[0x%x]: 0x%x\n", macro_idx, data);

			memset(&macro->istate, 0, sizeof(macro->istate));
			macro->istate.regs[1] = data;
			macro->istate.obj = current_subchan_object(pstate);
			macro->istate.code = macro->code + macro->entries[macro_idx].start / 4;
			macro->istate.words = macro->entries[macro_idx].words;
			macro->istate.delayed_pc = 0xffffffff;
			macro->istate.exit_when_0 = 0xffffffff;
			macro->istate.device = pstate->fifo;

			if (MMT_DEBUG && macro_dis_enabled)
			{
				struct varinfo *var = varinfo_new(isa_macro->vardata);

				envydis(isa_macro, stdout, (uint8_t *)macro->istate.code, 0,
						macro->istate.words, var, 0, NULL, 0, colors);
				varinfo_del(var);

				macro_dis(macro->istate.code, macro->istate.words,
						macro->istate.obj);
			}

			if (macro_rt)
				macro_sim(&macro->istate);
		}
		else
		{
			mmt_debug("MACRO_PARAM[0x%x]: 0x%x\n", macro_idx, data);
			if (macro_rt)
			{
				macro->istate.macro_param = &data;
				macro_sim(&macro->istate);
				macro->istate.macro_param = NULL;
			}
		}
	}
	else
		return 0;

	return 1;
}
