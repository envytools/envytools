/*
 * Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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

#ifndef NVRM_CLASS_H
#define NVRM_CLASS_H

#include <stdint.h>

struct nvrm_create_context {
	uint32_t cid;
};

struct nvrm_create_device {
	uint32_t idx;
	uint32_t cid;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};

struct nvrm_create_subdevice {
	uint32_t idx;
};

struct nvrm_create_event {
	uint32_t cid;
	uint32_t cls;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t ehandle;
	uint32_t unk14;
};

struct nvrm_create_unk83de {
	uint32_t unk00;
	uint32_t cid;
	uint32_t handle;
};

struct nvrm_create_unk0005 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};

struct nvrm_create_fifo_pio {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_fifo_dma {
	uint32_t handle1; // error_notify?
	uint32_t handle2; // dma?
	uint32_t user_addr;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};

struct nvrm_create_fifo_ib {
	uint32_t error_notify;
	uint32_t dma;
	uint64_t ib_addr;
	uint64_t ib_entries;
	uint32_t unk18;
	uint32_t unk1c;
};

struct nvrm_create_graph {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};

struct nvrm_create_copy {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_me {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_vp {
	uint32_t unk00[0x50/4];
};

struct nvrm_create_bsp {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_ppp {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_venc {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_sw_unk0075 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};

struct nvrm_create_sw_unk9072 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};

struct nvrm_create_disp_fifo {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_disp_fifo_pio {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};

struct nvrm_create_disp_fifo_dma {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};

struct nvrm_create_unk0078 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};

struct nvrm_create_unk007e {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};

struct nvrm_create_unk00db {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_unk00f1 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
};

struct nvrm_create_unk00ff {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
	uint32_t unk10;
	uint32_t unk14;
	uint32_t unk18;
	uint32_t unk1c;
};

struct nvrm_create_unk25a0 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};

struct nvrm_create_unk30f1 {
	uint32_t unk00;
};

struct nvrm_create_unk30f2 {
	uint32_t unk00;
};

struct nvrm_create_unk83f3 {
	uint32_t unk00;
};

struct nvrm_create_unk40ca {
	uint32_t unk00;
};

struct nvrm_create_unk503b {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_unk503c {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};

struct nvrm_create_unk5072 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};

struct nvrm_create_unk8d75 {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_unk906d {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_unk906e {
	uint32_t unk00;
	uint32_t unk04;
};

struct nvrm_create_unk90f1 {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
	uint32_t unk0c;
};

struct nvrm_create_unka06c {
	uint32_t unk00;
	uint32_t unk04;
	uint32_t unk08;
};

struct nvrm_create_unka0b7 {
	uint32_t unk00;
	uint32_t unk04;
};

#endif
