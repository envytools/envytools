/*
 * Copyright 2010 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#ifndef __NOUVEAU_PMS_H__
#define __NOUVEAU_PMS_H__

struct pms_ucode {
	u8 data[256];
	union {
		u8  *u08;
		u16 *u16;
		u32 *u32;
	} ptr;
	u16 len;

	u32 reg;
	u32 val;
};

static inline void
pms_init(struct pms_ucode *pms)
{
	pms->ptr.u08 = pms->data;
	pms->reg = 0xffffffff;
	pms->val = 0xffffffff;
}

static inline void
pms_fini(struct pms_ucode *pms)
{
	do {
		*pms->ptr.u08++ = 0x7f;
		pms->len = pms->ptr.u08 - pms->data;
	} while (pms->len & 3);
	pms->ptr.u08 = pms->data;
}

static inline void
pms_unkn(struct pms_ucode *pms, u8 v0)
{
	*pms->ptr.u08++ = v0;
}

static inline void
pms_op5f(struct pms_ucode *pms, u8 v0, u8 v1)
{
	*pms->ptr.u08++ = 0x5f;
	*pms->ptr.u08++ = v0;
	*pms->ptr.u08++ = v1;
}

static inline void
pms_wr32(struct pms_ucode *pms, u32 reg, u32 val)
{
	if (val != pms->val) {
		if ((val & 0xffff0000) == (pms->val & 0xffff0000)) {
			*pms->ptr.u08++ = 0x42;
			*pms->ptr.u16++ = (val & 0x0000ffff);
		} else {
			*pms->ptr.u08++ = 0xe2;
			*pms->ptr.u32++ = val;
		}

		pms->val = val;
	}

	if ((reg & 0xffff0000) == (pms->reg & 0xffff0000)) {
		*pms->ptr.u08++ = 0x40;
		*pms->ptr.u16++ = (reg & 0x0000ffff);
	} else {
		*pms->ptr.u08++ = 0xe0;
		*pms->ptr.u32++ = reg;
	}
	pms->reg = reg;
}

#endif
