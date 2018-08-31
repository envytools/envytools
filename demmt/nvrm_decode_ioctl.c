/*
 * Copyright (C) 2014 Marcin Ślusarz <marcin.slusarz@gmail.com>.
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "decode_utils.h"
#include "demmt.h"
#include "nvrm.h"
#include "nvrm_create.h"
#include "nvrm_decode.h"
#include "nvrm_ioctl.h"
#include "nvrm_mthd.h"
#include "nvrm_object.xml.h"
#include "nvrm_query.h"
#include "util.h"

#define _(V) { V, #V }
static struct status_description
{
	uint32_t val;
	const char *name;
}
status_descriptions[] =
{
	// verbose descriptions are in nvrm_ioctl.h
	_(NVRM_STATUS_ALREADY_EXISTS_SUB),
	_(NVRM_STATUS_ALREADY_EXISTS),
	_(NVRM_STATUS_INVALID_PARAM),
	_(NVRM_STATUS_INVALID_DEVICE),
	_(NVRM_STATUS_INVALID_MTHD),
	_(NVRM_STATUS_OBJECT_ERROR),
	_(NVRM_STATUS_NO_HW),
	_(NVRM_STATUS_MTHD_SIZE_MISMATCH),
	_(NVRM_STATUS_ADDRESS_FAULT),
	_(NVRM_STATUS_MTHD_CLASS_MISMATCH),
	_(NVRM_STATUS_MTHD_SIZE_MISMATCH2),
	_(NVRM_STATUS_SUCCESS),
};
#undef _

/* returns pointer to statically allocated memory, so don't cache */
const char *nvrm_status(uint32_t status)
{
	static char buf[128];
	int i;
	if (status == NVRM_STATUS_SUCCESS)
	{
		strcpy(buf, "SUCCESS");
		return buf;
	}

	for (i = 0; i < ARRAY_SIZE(status_descriptions); ++i)
		if (status_descriptions[i].val == status)
		{
			sprintf(buf, "%s%s%s", colors->err, status_descriptions[i].name +
					strlen("NVRM_STATUS_"), colors->reset);
			return buf;
		}

	sprintf(buf, "%s0x%08x%s", colors->err, status, colors->reset);
	return buf;
}

const char *nvrm_get_class_name(uint32_t cls)
{
	static struct rnnenum *rnndb_cls = NULL, *nvrm_cls = NULL;
	static uint32_t last_cls = 0;
	static const char *last_cls_name = NULL;

	if (!cls || !nvrm_describe_classes)
		return NULL;

	if (cls == last_cls)
		return last_cls_name;

	if (!rnndb_cls)
		rnndb_cls = rnn_findenum(rnndb, "obj-class");

	struct rnnvalue *v = NULL;
	FINDARRAY(rnndb_cls->vals, v, v->value == cls);
	if (v)
	{
		last_cls = cls;
		last_cls_name = v->name;
		return last_cls_name;
	}

	if (!nvrm_cls)
		nvrm_cls = rnn_findenum(rnndb_nvrm_object, "obj-class");

	v = NULL;
	FINDARRAY(nvrm_cls->vals, v, v->value == cls);
	if (v)
	{
		last_cls = cls;
		last_cls_name = v->name;
		return last_cls_name;
	}

	return NULL;
}

int nvrm_show_unk_zero_fields = 1;

int _nvrm_field_enabled(const char *name)
{
	if (!nvrm_show_unk_zero_fields && strncmp(name, "unk", 3) == 0)
		return 0;
	return 1;
}

const char *nvrm_sep = ", ";
const char *nvrm_pfx = "";

int nvrm_describe_handles = 1;
int nvrm_describe_classes = 1;

void describe_nvrm_object(uint32_t cid, uint32_t handle, const char *field_name)
{
	if (!nvrm_describe_handles)
		return;

	struct gpu_object *obj = gpu_object_find(cid, handle);

	if (!obj || strcmp(field_name, "cid") == 0)
		return;

	if (obj->cid == obj->handle)
	{
		mmt_log_cont(" [cid]%s", "");
		return;
	}

	if (obj->class_ == 0xffffffff)
	{
		mmt_log_cont(" [class: ?%s]", "");
		return;
	}

	const char *name = nvrm_get_class_name(obj->class_);
	if (name)
		mmt_log_cont(" [class: 0x%04x %s]", obj->class_, name);
	else
		mmt_log_cont(" [class: 0x%04x]", obj->class_);
}

static void decode_nvrm_ioctl_check_version_str(struct nvrm_ioctl_check_version_str *s)
{
	mmt_log_cont("cmd: %d, reply: %d, vernum: %s\n", s->cmd, s->reply, s->vernum);
}

static void decode_nvrm_ioctl_env_info(struct nvrm_ioctl_env_info *s)
{
	mmt_log_cont("pat_supported: %d\n", s->pat_supported);
}

static void decode_nvrm_ioctl_card_info(struct nvrm_ioctl_card_info *s)
{
	int nl = 0;
	int i, j;
	for (i = 0; i < 32; ++i)
	{
		int valid = 0;
		for (j = 0; j < sizeof(s->card[i]); ++j)
			if (((unsigned char *)&s->card[i])[j] != 0)
			{
				valid = 1;
				break;
			}
		if (valid)
		{
			if (!nl)
			{
				mmt_log_cont_nl();
				nl = 1;
			}

			mmt_log("    %d: ", i);
			nvrm_reset_pfx();
			nvrm_print_x32(&s->card[i], flags);
			nvrm_print_x32(&s->card[i], domain);
			nvrm_print_d32_align(&s->card[i], bus, 3);
			nvrm_print_d32_align(&s->card[i], slot, 3);
			nvrm_print_x16(&s->card[i], vendor_id);
			nvrm_print_x16(&s->card[i], device_id);
			nvrm_print_pad_x32(&s->card[i], _pad);
			nvrm_print_x32(&s->card[i], gpu_id);
			nvrm_print_x32(&s->card[i], interrupt);
			nvrm_print_pad_x32(&s->card[i], _pad2);
			nvrm_print_x64(&s->card[i], reg_address);
			nvrm_print_x64(&s->card[i], reg_size);
			nvrm_print_x64(&s->card[i], fb_address);
			nvrm_print_x64(&s->card[i], fb_size);
			mmt_log_cont_nl();
		}
	}
}

static void decode_nvrm_ioctl_card_info2(struct nvrm_ioctl_card_info2 *s)
{
	int nl = 0;
	int i, j;
	for (i = 0; i < 32; ++i)
	{
		int valid = 0;
		for (j = 0; j < sizeof(s->card[i]); ++j)
			if (((unsigned char *)&s->card[i])[j] != 0)
			{
				valid = 1;
				break;
			}
		if (valid)
		{
			if (!nl)
			{
				mmt_log_cont_nl();
				nl = 1;
			}

			mmt_log("    %d: ", i);
			nvrm_reset_pfx();
			nvrm_print_x32(&s->card[i], flags);
			nvrm_print_x32(&s->card[i], domain);
			nvrm_print_d32_align(&s->card[i], bus, 3);
			nvrm_print_d32_align(&s->card[i], slot, 3);
			nvrm_print_d32_align(&s->card[i], function, 3);
			nvrm_print_pad_x8(&s->card[i], _pad0);
			nvrm_print_x16(&s->card[i], vendor_id);
			nvrm_print_x16(&s->card[i], device_id);
			nvrm_print_pad_x32(&s->card[i], _pad1);
			nvrm_print_x32(&s->card[i], gpu_id);
			nvrm_print_x32(&s->card[i], interrupt);
			nvrm_print_pad_x32(&s->card[i], _pad2);
			nvrm_print_x64(&s->card[i], reg_address);
			nvrm_print_x64(&s->card[i], reg_size);
			nvrm_print_x64(&s->card[i], fb_address);
			nvrm_print_x64(&s->card[i], fb_size);
			nvrm_print_d32(&s->card[i], index);
			nvrm_print_pad_x32(&s->card[i], _pad3);
			mmt_log_cont_nl();
		}
	}
}

static void decode_nvrm_ioctl_card_info3(struct nvrm_ioctl_card_info3 *s)
{
	int nl = 0;
	int i, j;
	for (i = 0; i < 32; ++i)
	{
		int valid = 0;
		for (j = 0; j < sizeof(s->card[i]); ++j)
			if (((unsigned char *)&s->card[i])[j] != 0)
			{
				valid = 1;
				break;
			}
		if (valid)
		{
			if (!nl)
			{
				mmt_log_cont_nl();
				nl = 1;
			}

			mmt_log("    %d: ", i);
			nvrm_reset_pfx();
			nvrm_print_x32(&s->card[i], flags);
			nvrm_print_x32(&s->card[i], domain);
			nvrm_print_d32_align(&s->card[i], bus, 3);
			nvrm_print_d32_align(&s->card[i], slot, 3);
			nvrm_print_d32_align(&s->card[i], function, 3);
			nvrm_print_pad_x8(&s->card[i], _pad0);
			nvrm_print_x16(&s->card[i], vendor_id);
			nvrm_print_x16(&s->card[i], device_id);
			nvrm_print_pad_x32(&s->card[i], _pad1);
			nvrm_print_x32(&s->card[i], gpu_id);
			nvrm_print_x32(&s->card[i], interrupt);
			nvrm_print_pad_x32(&s->card[i], _pad2);
			nvrm_print_x64(&s->card[i], reg_address);
			nvrm_print_x64(&s->card[i], reg_size);
			nvrm_print_x64(&s->card[i], fb_address);
			nvrm_print_x64(&s->card[i], fb_size);
			nvrm_print_d32(&s->card[i], index);
			nvrm_print_pad_x32(&s->card[i], _pad3);
			nvrm_print_pad_x32(&s->card[i], _pad4);
			nvrm_print_pad_x32(&s->card[i], _pad5);
			mmt_log_cont_nl();
		}
	}
}

static void decode_create_fifo_dma(struct nvrm_ioctl_create *i, struct nvrm_create_fifo_dma *s)
{
	nvrm_print_x32(s, handle1);
	nvrm_print_x32(s, handle2);
	nvrm_print_x32(s, user_addr);
	nvrm_print_x32(s, unk0c);
	nvrm_print_x32(s, unk10);
	nvrm_print_x32(s, unk14);
	nvrm_print_x32(s, unk18);
	nvrm_print_x32(s, unk1c);
	mmt_log_cont_nl();
}

static void decode_create_fifo_ib(struct nvrm_ioctl_create *i, struct nvrm_create_fifo_ib *s)
{
	nvrm_print_x32(s, error_notify);
	nvrm_print_x32(s, dma);
	nvrm_print_x64(s, ib_addr);
	nvrm_print_x64(s, ib_entries);
	nvrm_print_x32(s, unk18);
	nvrm_print_x32(s, unk1c);
	mmt_log_cont_nl();
}

static void decode_create_context(struct nvrm_ioctl_create *i, struct nvrm_create_context *s)
{
	nvrm_print_cid(s, cid);
	mmt_log_cont_nl();
}

static void decode_create_event(struct nvrm_ioctl_create *i, struct nvrm_create_event *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_class(s, cls);
	nvrm_print_x32(s, unk08);
	nvrm_print_x32(s, unk0c);
	nvrm_print_handle(s, ehandle, cid);
	nvrm_print_x32(s, unk14);
	mmt_log_cont_nl();
}

struct nvrm_create_arg_decoder
{
	uint32_t cls;
	int size;
	void *fun;
};

#define _(CLS, STR, FUN) { CLS, sizeof(STR), FUN }
struct nvrm_create_arg_decoder nvrm_create_arg_decoders[] =
{
		_(NVRM_FIFO_DMA_NV40, struct nvrm_create_fifo_dma, decode_create_fifo_dma),
		_(NVRM_FIFO_DMA_NV44, struct nvrm_create_fifo_dma, decode_create_fifo_dma),
		_(NVRM_FIFO_IB_G80, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_G82, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_MCP89, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_GF100, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_GK104, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_GK110, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_UNKA2, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_GM107, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_FIFO_IB_UNKC0, struct nvrm_create_fifo_ib, decode_create_fifo_ib),
		_(NVRM_CONTEXT_NEW, struct nvrm_create_context, decode_create_context),
		_(NVRM_CONTEXT, struct nvrm_create_context, decode_create_context),
		_(NVRM_EVENT, struct nvrm_create_event, decode_create_event),
};
#undef _
int nvrm_create_arg_decoder_cnt = ARRAY_SIZE(nvrm_create_arg_decoders);

static void decode_nvrm_ioctl_create(struct nvrm_ioctl_create *s, struct mmt_memory_dump *args, int argc)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_class(s, cls);
	nvrm_print_handle(s, parent, cid);
	struct mmt_buf *data = nvrm_print_ptr(s, ptr, args, argc);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ln();

	if (data)
	{
		int found = 0;

		void (*fun)(struct nvrm_ioctl_create *, void *) = NULL;

		struct nvrm_create_arg_decoder *dec;
		int k;
		for (k = 0; k < nvrm_create_arg_decoder_cnt; ++k)
		{
			dec = &nvrm_create_arg_decoders[k];
			if (dec->cls == s->cls)
			{
				if (dec->size == data->len)
				{
					nvrm_reset_pfx();
					mmt_log("        %s", "ptr[]: ");
					fun = dec->fun;
					if (fun)
						fun(s, (void *)data->data);

					found = 1;
				}
				break;
			}
		}

		if (!found)
			dump_mmt_buf_as_words_horiz(data, "ptr[]:");
	}
}

static void decode_nvrm_ioctl_config(struct nvrm_ioctl_config *s, struct mmt_memory_dump *args, int argc)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x64(s, unk08);
	nvrm_print_x64(s, unk10);
	nvrm_print_x64(s, slen);
	struct mmt_buf *data = nvrm_print_ptr(s, sptr, args, argc);
	nvrm_print_x64(s, unk28);
	nvrm_print_x64(s, unk30);
	nvrm_print_x64(s, unk38);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ln();

	if (data)
	{
		int i;
		mmt_log("        sptr[]: \"%s", "");
		for (i = 0; i < data->len; ++i)
			mmt_log_cont("%c", ((char *)data->data)[i]);
		mmt_log_cont("\"%s\n", "");
	}
}

static void decode_nvrm_ioctl_create_vspace(struct nvrm_ioctl_create_vspace *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_class(s, cls);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_x32(s, flags);
	nvrm_print_pad_x32(s, _pad1);
	nvrm_print_x64(s, foffset);
	nvrm_print_x64(s, limit);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_vspace56(struct nvrm_ioctl_create_vspace56 *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_class(s, cls);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_x32(s, flags);
	nvrm_print_pad_x32(s, _pad1);
	nvrm_print_x64(s, map_id);
	nvrm_print_x64(s, limit);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_d32(s, target_fd);
	nvrm_print_pad_x32(s, _pad3);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_dma(struct nvrm_ioctl_create_dma *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_class(s, cls);
	nvrm_print_x32(s, flags);
	nvrm_print_pad_x32(s, _pad1);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, limit);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_dma56(struct nvrm_ioctl_create_dma56 *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_pad_x32(s, _pad1);
	nvrm_print_class(s, cls);
	nvrm_print_x32(s, flags);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_pad_x32(s, _pad3);
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, limit);
	nvrm_print_status(s, status);
	nvrm_print_x32(s, unk34);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_host_map(struct nvrm_ioctl_host_map *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, subdev, cid);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, limit);
	nvrm_print_x64(s, foffset);
	nvrm_print_status(s, status);
	nvrm_print_x32(s, unk);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_host_map56(struct nvrm_ioctl_host_map56 *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, subdev, cid);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_x64(s, foffset);
	nvrm_print_x64(s, length);
	nvrm_print_x64(s, map_id);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_d32(s, target_fd);
	nvrm_print_pad_x32(s, _pad3);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_destroy(struct nvrm_ioctl_destroy *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_vspace_map(struct nvrm_ioctl_vspace_map *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, dev, cid);
	nvrm_print_handle(s, vspace, cid);
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, size);
	nvrm_print_x32(s, flags);
	nvrm_print_pad_x32(s, _pad1);
	nvrm_print_x64(s, addr);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_host_unmap(struct nvrm_ioctl_host_unmap *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, subdev, cid);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_x64(s, foffset);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_memory(struct nvrm_ioctl_memory *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_class(s, cls);
	nvrm_print_x32(s, unk0c);
	nvrm_print_status(s, status);
	nvrm_print_x32(s, unk14);
	nvrm_print_x64(s, vram_total);
	nvrm_print_x64(s, vram_free);
	nvrm_print_handle(s, vspace, cid);
	nvrm_print_x32(s, unk30);
	nvrm_print_x32(s, flags1);
	nvrm_print_x64(s, unk38);
	nvrm_print_x32(s, flags2);
	nvrm_print_x32(s, unk44);
	nvrm_print_x64(s, unk48);
	nvrm_print_x32(s, flags3);
	nvrm_print_x32(s, unk54);
	nvrm_print_x64(s, unk58);
	nvrm_print_x64(s, size);
	nvrm_print_x64(s, align);
	nvrm_print_x64(s, base);
	nvrm_print_x64(s, limit);
	nvrm_print_x64(s, unk80);
	nvrm_print_x64(s, unk88);
	nvrm_print_x64(s, unk90);
	nvrm_print_x64(s, unk98);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_memory2(struct nvrm_ioctl_memory2 *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_class(s, cls);
	nvrm_print_x32(s, unk0c);
	nvrm_print_status(s, status);
	nvrm_print_x32(s, unk14);
	nvrm_print_x64(s, vram_total);
	nvrm_print_x64(s, vram_free);
	nvrm_print_handle(s, vspace, cid);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_vspace_unmap(struct nvrm_ioctl_vspace_unmap *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, dev, cid);
	nvrm_print_handle(s, vspace, cid);
	nvrm_print_x64(s, unk10);
	nvrm_print_x64(s, addr);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_unk5e(struct nvrm_ioctl_unk5e *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, subdev, cid);
	nvrm_print_pad_x32(s, _pad1);
	nvrm_print_x64(s, foffset);
	nvrm_print_x64(s, ptr);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad2);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_bind(struct nvrm_ioctl_bind *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, target, cid);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_os_event(struct nvrm_ioctl_create_os_event *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, ehandle, cid);
	nvrm_print_d32(s, fd);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_destroy_os_event(struct nvrm_ioctl_destroy_os_event *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_d32(s, fd);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_simple(struct nvrm_ioctl_create_simple *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_class(s, cls);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_get_param(struct nvrm_ioctl_get_param *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x32(s, key);
	nvrm_print_x32(s, value);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_unk34(struct nvrm_ioctl_create_unk34 *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_cid(s, cid2);
	nvrm_print_handle(s, handle2, cid2);
	nvrm_print_x32(s, unk14);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_unk38(struct nvrm_ioctl_unk38 *s, struct mmt_memory_dump *args, int argc)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x32(s, unk08);
	nvrm_print_x32(s, size);
	struct mmt_buf *data = nvrm_print_ptr(s, ptr, args, argc);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words_horiz(data, "ptr[]:");
}

static void decode_nvrm_ioctl_sched_fifo(struct nvrm_ioctl_sched_fifo *s, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data1, *data2, *data3;

	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, dev, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x32(s, cnt);
	data1 = nvrm_print_ptr(s, ptr1, args, argc);
	data2 = nvrm_print_ptr(s, ptr2, args, argc);
	data3 = nvrm_print_ptr(s, ptr3, args, argc);
	nvrm_print_x32(s, unk28);
	nvrm_print_x32(s, unk2c);
	nvrm_print_status(s, status);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ln();

	if (data1)
		dump_mmt_buf_as_words_horiz(data1, "ptr1: ");
	if (data2)
		dump_mmt_buf_as_words_horiz(data2, "ptr2: ");
	if (data3)
		dump_mmt_buf_as_words_horiz(data3, "ptr3: ");
}

static void decode_nvrm_ioctl_disp_unk48(struct nvrm_ioctl_disp_unk48 *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x32(s, unk08);
	nvrm_print_pad_x32(s, _pad);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_unk52(struct nvrm_ioctl_unk52 *s, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data = nvrm_print_ptr(s, ptr, args, argc);
	nvrm_print_x32(s, unk08);
	nvrm_print_status(s, status);
	nvrm_print_ln();

	if (data)
		dump_mmt_buf_as_words_horiz(data, "ptr[]:");
}

static void decode_nvrm_ioctl_create_ctx(struct nvrm_ioctl_create_ctx *s)
{
	nvrm_print_handle(s, handle, handle);
	nvrm_print_x32(s, unk04);
	nvrm_print_x32(s, unk08);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_dev_obj(struct nvrm_ioctl_create_dev_obj *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_x32(s, unk08);
	nvrm_print_x32(s, unk0c);
	nvrm_print_x32(s, ptr);
	nvrm_print_x32(s, unk14);
	nvrm_print_x32(s, unk18);
	nvrm_print_x32(s, unk1c);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_create_drv_obj(struct nvrm_ioctl_create_drv_obj *s)
{
	nvrm_print_cid(s, cid);
	nvrm_print_handle(s, handle, cid);
	nvrm_print_class(s, cls);
	nvrm_print_handle(s, parent, cid);
	nvrm_print_status(s, status);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_unkd6(struct nvrm_ioctl_unkd6 *s)
{
	nvrm_print_x32(s, unk00);
	nvrm_print_x32(s, unk04);
	nvrm_print_ln();
}

static void decode_nvrm_ioctl_unkd7(struct nvrm_ioctl_unkd7 *s)
{
	nvrm_print_x32(s, unk00);
	nvrm_print_x32(s, unk04);
	nvrm_print_x32(s, unk08);
	nvrm_print_x32(s, unk0c);
	nvrm_print_x32(s, unk10);
	nvrm_print_x32(s, unk14);
	nvrm_print_x32(s, unk18);
	nvrm_print_x32(s, unk1c);
	nvrm_print_ln();
}

#define _(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), FUN, NULL, 0 }
#define _a(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), NULL, FUN, 0 }
struct nvrm_ioctl nvrm_ioctls[] =
{
		_(NVRM_IOCTL_UNKD6, struct nvrm_ioctl_unkd6, decode_nvrm_ioctl_unkd6),
		_(NVRM_IOCTL_UNKD7, struct nvrm_ioctl_unkd7, decode_nvrm_ioctl_unkd7),
		_(NVRM_IOCTL_CHECK_VERSION_STR, struct nvrm_ioctl_check_version_str, decode_nvrm_ioctl_check_version_str),
		_(NVRM_IOCTL_ENV_INFO, struct nvrm_ioctl_env_info, decode_nvrm_ioctl_env_info),
		_(NVRM_IOCTL_CARD_INFO, struct nvrm_ioctl_card_info, decode_nvrm_ioctl_card_info),
		_(NVRM_IOCTL_CARD_INFO2, struct nvrm_ioctl_card_info2, decode_nvrm_ioctl_card_info2),
		_(NVRM_IOCTL_CARD_INFO3, struct nvrm_ioctl_card_info3, decode_nvrm_ioctl_card_info3),
		_a(NVRM_IOCTL_CREATE, struct nvrm_ioctl_create, decode_nvrm_ioctl_create),
		_a(NVRM_IOCTL_CALL, struct nvrm_ioctl_call, decode_nvrm_ioctl_call),
		_a(NVRM_IOCTL_CONFIG, struct nvrm_ioctl_config, decode_nvrm_ioctl_config),
		_(NVRM_IOCTL_CREATE_VSPACE, struct nvrm_ioctl_create_vspace, decode_nvrm_ioctl_create_vspace),
		_(NVRM_IOCTL_CREATE_VSPACE56, struct nvrm_ioctl_create_vspace56, decode_nvrm_ioctl_create_vspace56),
		_(NVRM_IOCTL_CREATE_DMA, struct nvrm_ioctl_create_dma, decode_nvrm_ioctl_create_dma),
		_(NVRM_IOCTL_CREATE_DMA56, struct nvrm_ioctl_create_dma56, decode_nvrm_ioctl_create_dma56),
		_(NVRM_IOCTL_HOST_MAP, struct nvrm_ioctl_host_map, decode_nvrm_ioctl_host_map),
		_(NVRM_IOCTL_HOST_MAP56, struct nvrm_ioctl_host_map56, decode_nvrm_ioctl_host_map56),
		_(NVRM_IOCTL_DESTROY, struct nvrm_ioctl_destroy, decode_nvrm_ioctl_destroy),
		_(NVRM_IOCTL_VSPACE_MAP, struct nvrm_ioctl_vspace_map, decode_nvrm_ioctl_vspace_map),
		_(NVRM_IOCTL_HOST_UNMAP, struct nvrm_ioctl_host_unmap, decode_nvrm_ioctl_host_unmap),
		_(NVRM_IOCTL_MEMORY, struct nvrm_ioctl_memory, decode_nvrm_ioctl_memory),
		_(NVRM_IOCTL_MEMORY2, struct nvrm_ioctl_memory2, decode_nvrm_ioctl_memory2),
		_(NVRM_IOCTL_VSPACE_UNMAP, struct nvrm_ioctl_vspace_unmap, decode_nvrm_ioctl_vspace_unmap),
		_(NVRM_IOCTL_UNK5E, struct nvrm_ioctl_unk5e, decode_nvrm_ioctl_unk5e),
		_(NVRM_IOCTL_BIND, struct nvrm_ioctl_bind, decode_nvrm_ioctl_bind),
		_(NVRM_IOCTL_CREATE_OS_EVENT, struct nvrm_ioctl_create_os_event, decode_nvrm_ioctl_create_os_event),
		_(NVRM_IOCTL_DESTROY_OS_EVENT, struct nvrm_ioctl_destroy_os_event, decode_nvrm_ioctl_destroy_os_event),
		_(NVRM_IOCTL_CREATE_SIMPLE, struct nvrm_ioctl_create_simple, decode_nvrm_ioctl_create_simple),
		_(NVRM_IOCTL_GET_PARAM, struct nvrm_ioctl_get_param, decode_nvrm_ioctl_get_param),
		_a(NVRM_IOCTL_QUERY, struct nvrm_ioctl_query, decode_nvrm_ioctl_query),
		_(NVRM_IOCTL_CREATE_UNK34, struct nvrm_ioctl_create_unk34, decode_nvrm_ioctl_create_unk34),
		_a(NVRM_IOCTL_UNK38, struct nvrm_ioctl_unk38, decode_nvrm_ioctl_unk38),
		_a(NVRM_IOCTL_SCHED_FIFO, struct nvrm_ioctl_sched_fifo, decode_nvrm_ioctl_sched_fifo),
		_(NVRM_IOCTL_DISP_UNK48, struct nvrm_ioctl_disp_unk48, decode_nvrm_ioctl_disp_unk48),
		_a(NVRM_IOCTL_UNK52, struct nvrm_ioctl_unk52, decode_nvrm_ioctl_unk52),
		_(NVRM_IOCTL_CREATE_CTX, struct nvrm_ioctl_create_ctx, decode_nvrm_ioctl_create_ctx),
		_(NVRM_IOCTL_CREATE_DEV_OBJ, struct nvrm_ioctl_create_dev_obj, decode_nvrm_ioctl_create_dev_obj),
		_(NVRM_IOCTL_CREATE_DRV_OBJ, struct nvrm_ioctl_create_drv_obj, decode_nvrm_ioctl_create_drv_obj),
};
#undef _
#undef _a
int nvrm_ioctls_cnt = ARRAY_SIZE(nvrm_ioctls);

static int decode_nvrm_ioctl(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr,
		uint16_t size, struct mmt_buf *buf, uint64_t ret, uint64_t err,
		void *state, struct mmt_memory_dump *args, int argc, const char *name)
{
	int k, found = 0;
	int args_used = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	struct nvrm_ioctl *ioctl;
	for (k = 0; k < nvrm_ioctls_cnt; ++k)
	{
		ioctl = &nvrm_ioctls[k];
		if (ioctl->id == id)
		{
			if (ioctl->size == buf->len)
			{
				if (dump_decoded_ioctl_data && !ioctl->disabled)
				{
					mmt_log("%-26s %-5s fd: %d, ", ioctl->name, name, fd);
					if (ret)
						mmt_log_cont("ret: %" PRId64 ", ", ret);
					if (err)
						mmt_log_cont("%serr: %" PRId64 "%s, ", colors->err, err, colors->reset);

					nvrm_reset_pfx();
					fun = ioctl->fun;
					if (fun)
						fun(buf->data);
					fun_with_args = ioctl->fun_with_args;
					if (fun_with_args)
					{
						fun_with_args(buf->data, args, argc);
						args_used = 1;
					}
				}
				found = 1;
			}
			break;
		}
	}

	if (!found)
		return 1;

	if ((!args_used && dump_decoded_ioctl_data && !ioctl->disabled) || dump_raw_ioctl_data)
		dump_args(args, argc, 0);

	return 0;
}

int decode_nvrm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_nvrm_ioctl(fd, id, dir, nr, size, buf, 0, 0, state, args, argc, "pre,");
}

int decode_nvrm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc)
{
	return decode_nvrm_ioctl(fd, id, dir, nr, size, buf, ret, err, state, args, argc, "post,");
}
