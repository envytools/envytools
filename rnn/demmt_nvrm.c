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

#include "demmt.h"
#include "demmt_pushbuf.h"
#include "demmt_nvrm.h"
#include "nvrm_ioctl.h"
#include "nvrm_mthd.h"
#include "nvrm_query.h"
#include "util.h"
#include <string.h>

struct nvrm_object
{
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t class_;
	struct nvrm_object *next;
};

#define NVRM_OBJECT_CACHE_SIZE (512 + 1)
static struct nvrm_object *nvrm_object_cache[NVRM_OBJECT_CACHE_SIZE];
static void nvrm_add_object(uint32_t cid, uint32_t parent, uint32_t handle, uint32_t class_)
{
	int bucket = (handle * (handle + 3)) % NVRM_OBJECT_CACHE_SIZE;

	struct nvrm_object *entry = nvrm_object_cache[bucket];
	while (entry && (entry->cid != cid || entry->handle != handle))
		entry = entry->next;

	if (entry)
	{
		mmt_error("object 0x%08x / 0x%08x already exists!\n", cid, handle);
		if (entry->class_ != class_ || entry->parent != parent)
		{
			mmt_error("... and its class or parent differ! 0x%04x != 0x%04x || 0x%08x != 0x%08x\n",
					entry->class_, class_, entry->parent, parent);
			fflush(stdout);
			abort();
		}
	}
	else
	{
		if (0 && nvrm_object_cache[bucket])
			mmt_error("collision%s\n", "");

		entry = malloc(sizeof(struct nvrm_object));
		entry->cid = cid;
		entry->parent = parent;
		entry->handle = handle;
		entry->class_ = class_;
		entry->next = nvrm_object_cache[bucket];
		nvrm_object_cache[bucket] = entry;
	}
}

static void nvrm_del_object(uint32_t cid, uint32_t parent, uint32_t handle)
{
	int bucket = (handle * (handle + 3)) % NVRM_OBJECT_CACHE_SIZE;

	struct nvrm_object *entry = nvrm_object_cache[bucket];
	struct nvrm_object *preventry = NULL;
	while (entry && (entry->cid != cid || entry->handle != handle))
	{
		preventry = entry;
		entry = entry->next;
	}

	if (!entry)
	{
		mmt_error("trying to delete object 0x%08x / 0x%08x which does not exist!\n", cid, handle);
		return;
	}

	if (preventry)
		preventry->next = entry->next;
	else
		nvrm_object_cache[bucket] = entry->next;

	free(entry);
}

static struct nvrm_object *nvrm_get_object(uint32_t cid, uint32_t handle)
{
	int bucket = (handle * (handle + 3)) % NVRM_OBJECT_CACHE_SIZE;

	struct nvrm_object *entry = nvrm_object_cache[bucket];
	while (entry && (entry->cid != cid || entry->handle != handle))
		entry = entry->next;

	return entry;
}

static struct mmt_buf *find_ptr(uint64_t ptr, struct mmt_memory_dump *args, int argc)
{
	int i;
	for (i = 0; i < argc; ++i)
		if (args[i].addr == ptr)
			return args[i].data;

	return NULL;
}

static void dump_mmt_buf_as_words(struct mmt_buf *buf)
{
	int j;
	for (j = 0; j < buf->len / 4; ++j)
		mmt_log("        0x%08x\n", ((uint32_t *)buf->data)[j]);
	for (j = buf->len / 4 * 4; j < buf->len; ++j)
		mmt_log("        0x%02x\n", buf->data[j]);
}

static void dump_mmt_buf_as_words_horiz(struct mmt_buf *buf, const char *pfx)
{
	int j;
	mmt_log("        %s", pfx ? pfx : "");
	for (j = 0; j < buf->len / 4; ++j)
		mmt_log_cont(" 0x%08x", ((uint32_t *)buf->data)[j]);
	for (j = buf->len / 4 * 4; j < buf->len; ++j)
		mmt_log_cont(" 0x%02x\n", buf->data[j]);
	mmt_log_cont_nl();
}

static void dump_mmt_buf_as_word_pairs(struct mmt_buf *buf, const char *(*fun)(uint32_t))
{
	int j;
	uint32_t *data = (uint32_t *)buf->data;
	for (j = 0; j < buf->len / 4 / 2; ++j)
	{
		const char *tmp = NULL;
		if (fun)
			tmp = fun(data[j * 2]);

		if (tmp)
			mmt_log("        0x%08x -> 0x%08x   [%s]\n", data[j * 2], data[j * 2 + 1], tmp);
		else
			mmt_log("        0x%08x -> 0x%08x\n", data[j * 2], data[j * 2 + 1]);
	}
}

static void dump_mmt_buf_as_words_desc(struct mmt_buf *buf, const char *(*fun)(uint32_t))
{
	int j;
	uint32_t *data = (uint32_t *)buf->data;
	for (j = 0; j < buf->len / 4; ++j)
	{
		const char *tmp = NULL;
		if (fun)
			tmp = fun(data[j]);

		if (tmp)
			mmt_log("        0x%08x   [%s]\n", data[j], tmp);
		else
			mmt_log("        0x%08x\n", data[j]);
	}
}

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
	_(NVRM_STATUS_SUCCESS),
};
#undef _

/* returns pointer to statically allocated memory, so don't cache */
static const char *nvrm_status(uint32_t status)
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

const char *demmt_nvrm_get_class_name(uint32_t cls)
{
	if (!cls)
		return NULL;

	struct rnnenum *cls_ = rnn_findenum(rnndb, "obj-class");
	struct rnnvalue *v = NULL;
	FINDARRAY(cls_->vals, v, v->value == cls);
	if (v)
		return v->name;

	cls_ = rnn_findenum(rnndb_nvrm_object, "obj-class");
	v = NULL;
	FINDARRAY(cls_->vals, v, v->value == cls);
	if (v)
		return v->name;

	return NULL;
}

static const char *sep = ", ";
static const char *pfx = "";

#define print_u64(strct, field)				do { mmt_log_cont("%s" #field ": 0x%016lx", pfx, strct->field); pfx = sep; } while (0)
#define print_u32(strct, field)				do { mmt_log_cont("%s" #field ": 0x%08x",   pfx, strct->field); pfx = sep; } while (0)
#define print_u16(strct, field)				do { mmt_log_cont("%s" #field ": 0x%04x",   pfx, strct->field); pfx = sep; } while (0)

#define print_i32(strct, field)				do { mmt_log_cont("%s" #field ": %d", pfx, strct->field); pfx = sep; } while (0)
#define print_i32_align(strct, field, algn)	do { mmt_log_cont("%s" #field ": %" #algn "d", pfx, strct->field); pfx = sep; } while (0)

#define print_pad_u32(strct, field)			do { if (strct->field) mmt_log_cont("%s%s" #field ": 0x%08x%s",   pfx, colors->err, strct->field, colors->reset); pfx = sep; } while (0)

#define print_str(strct, field)				do { mmt_log_cont("%s" #field ": \"%s\"",   pfx, strct->field); pfx = sep; } while (0)

#define print_ptr(strct, field, args, argc) \
	({ \
		struct mmt_buf *__ret = NULL; \
		if (strct->field) { \
			mmt_log_cont("%s" #field ": 0x%016lx", pfx, strct->field); \
			if (argc > 0) \
				__ret = find_ptr(strct->field, args, argc); \
			if (__ret == NULL) \
				mmt_log_cont("%s", " [no data]"); \
		} else \
			mmt_log_cont("%s" #field ": %s", pfx, "NULL"); \
		pfx = sep; \
		__ret; \
	})

static void describe_nvrm_object(uint32_t cid, uint32_t handle, const char *field_name)
{
	struct nvrm_object *obj = nvrm_get_object(cid, handle);

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

	const char *name = demmt_nvrm_get_class_name(obj->class_);
	if (name)
		mmt_log_cont(" [class: 0x%04x %s]", obj->class_, name);
	else
		mmt_log_cont(" [class: 0x%04x]", obj->class_);
}

#define print_handle(strct, field, cid) \
	do { \
		print_u32(strct, field); \
		describe_nvrm_object(strct->cid, strct->field, #field); \
	} while (0)

#define print_cid(strct, field)		print_u32(strct, field)

#define print_class(strct, field) \
	do { \
		mmt_log_cont("%sclass: 0x%04x", pfx, strct->field); \
		const char *__n = demmt_nvrm_get_class_name(strct->field); \
		if (__n) \
			mmt_log_cont(" [%s]", __n); \
		pfx = sep; \
	} while (0)

#define print_status(strct, field)			do { mmt_log_cont("%s" #field ": %s", pfx, nvrm_status(strct->field)); pfx = sep; } while (0)
#define print_ln() mmt_log_cont_nl()

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

			mmt_log("    %d: flags: 0x%08x, domain: 0x%08x, bus: %3d, slot: %3d, "
					"vendor_id: 0x%04x, device_id: 0x%04x, gpu_id: 0x%08x, interrupt: 0x%08x, reg_address: 0x%016lx, "
					"reg_size: 0x%016lx, fb_address: 0x%016lx, fb_size: 0x%016lx\n", i,
					s->card[i].flags, s->card[i].domain, s->card[i].bus, s->card[i].slot,
					s->card[i].vendor_id, s->card[i].device_id,
					s->card[i].gpu_id, s->card[i].interrupt, s->card[i].reg_address,
					s->card[i].reg_size, s->card[i].fb_address, s->card[i].fb_size);
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

			mmt_log("    %d: flags: 0x%08x, domain: 0x%08x, bus: %3d, slot: %3d, function: %3d, "
					"vendor_id: 0x%04x, device_id: 0x%04x, gpu_id: 0x%08x, interrupt: 0x%08x, reg_address: 0x%016lx, "
					"reg_size: 0x%016lx, fb_address: 0x%016lx, fb_size: 0x%016lx, index: %d\n", i,
					s->card[i].flags, s->card[i].domain, s->card[i].bus, s->card[i].slot,
					s->card[i].function, s->card[i].vendor_id, s->card[i].device_id,
					s->card[i].gpu_id, s->card[i].interrupt, s->card[i].reg_address,
					s->card[i].reg_size, s->card[i].fb_address, s->card[i].fb_size,
					s->card[i].index);
		}
	}
}

static void decode_nvrm_ioctl_create(struct nvrm_ioctl_create *s, struct mmt_memory_dump *args, int argc)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_handle(s, handle, cid);
	print_class(s, cls);
	struct mmt_buf *data = print_ptr(s, ptr, args, argc);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();

	if (data)
		dump_mmt_buf_as_words_horiz(data, "ptr[]:");
}

static void decode_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, mthd);
	print_pad_u32(s, _pad);
	print_ptr(s, ptr, args, argc);
	print_u32(s, size);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_unk4d(struct nvrm_ioctl_unk4d *s, struct mmt_memory_dump *args, int argc)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u64(s, unk08);
	print_u64(s, unk10);
	print_u64(s, slen);
	struct mmt_buf *data = print_ptr(s, sptr, args, argc);
	print_u64(s, unk28);
	print_u64(s, unk30);
	print_u64(s, unk38);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();

	if (data)
	{
		int i;
		mmt_log("        sptr[]: \"%s", "");
		for (i = 0; i < data->len; ++i)
			mmt_log_cont("%c", ((char *)data->data)[i]);
		mmt_log_cont("\"\n%s", "");
	}
}

static void decode_nvrm_ioctl_create_vspace(struct nvrm_ioctl_create_vspace *s)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_handle(s, handle, cid);
	print_class(s, cls);
	print_u32(s, flags);
	print_pad_u32(s, _pad1);
	print_u64(s, foffset);
	print_u64(s, limit);
	print_status(s, status);
	print_pad_u32(s, _pad2);
	print_ln();
}

static void decode_nvrm_ioctl_create_dma(struct nvrm_ioctl_create_dma *s)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_class(s, cls);
	print_u32(s, flags);
	print_pad_u32(s, _pad1);
	print_handle(s, parent, cid);
	print_u64(s, base);
	print_u64(s, limit);
	print_status(s, status);
	print_pad_u32(s, _pad2);
	print_ln();
}

static void decode_nvrm_ioctl_host_map(struct nvrm_ioctl_host_map *s)
{
	print_cid(s, cid);
	print_handle(s, subdev, cid);
	print_handle(s, handle, cid);
	print_pad_u32(s, _pad);
	print_u64(s, base);
	print_u64(s, limit);
	print_u64(s, foffset);
	print_status(s, status);
	print_u32(s, unk);
	print_ln();
}

static void decode_nvrm_ioctl_destroy(struct nvrm_ioctl_destroy *s)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_handle(s, handle, cid);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_vspace_map(struct nvrm_ioctl_vspace_map *s)
{
	print_cid(s, cid);
	print_handle(s, dev, cid);
	print_handle(s, vspace, cid);
	print_handle(s, handle, cid);
	print_u64(s, base);
	print_u64(s, size);
	print_u32(s, flags);
	print_pad_u32(s, _pad1);
	print_u64(s, addr);
	print_status(s, status);
	print_pad_u32(s, _pad2);
	print_ln();
}

static void decode_nvrm_ioctl_host_unmap(struct nvrm_ioctl_host_unmap *s)
{
	print_cid(s, cid);
	print_handle(s, subdev, cid);
	print_handle(s, handle, cid);
	print_pad_u32(s, _pad);
	print_u64(s, foffset);
	print_status(s, status);
	print_pad_u32(s, _pad2);
	print_ln();
}

static void decode_nvrm_ioctl_memory(struct nvrm_ioctl_memory *s)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_class(s, cls);
	print_u32(s, unk0c);
	print_status(s, status);
	print_u32(s, unk14);
	print_u64(s, vram_total);
	print_u64(s, vram_free);
	print_handle(s, vspace, cid);
	print_handle(s, handle, cid);
	print_u32(s, unk30);
	print_u32(s, flags1);
	print_u64(s, unk38);
	print_u32(s, flags2);
	print_u32(s, unk44);
	print_u64(s, unk48);
	print_u32(s, flags3);
	print_u32(s, unk54);
	print_u64(s, unk58);
	print_u64(s, size);
	print_u64(s, align);
	print_u64(s, base);
	print_u64(s, limit);
	print_u64(s, unk80);
	print_u64(s, unk88);
	print_u64(s, unk90);
	print_u64(s, unk98);
	print_ln();
}

static void decode_nvrm_ioctl_vspace_unmap(struct nvrm_ioctl_vspace_unmap *s)
{
	print_cid(s, cid);
	print_handle(s, dev, cid);
	print_handle(s, vspace, cid);
	print_handle(s, handle, cid);
	print_u64(s, unk10);
	print_u64(s, addr);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();
}

static void decode_nvrm_ioctl_unk5e(struct nvrm_ioctl_unk5e *s)
{
	print_cid(s, cid);
	print_handle(s, subdev, cid);
	print_handle(s, handle, cid);
	print_pad_u32(s, _pad1);
	print_u64(s, foffset);
	print_u64(s, ptr);
	print_status(s, status);
	print_pad_u32(s, _pad2);
	print_ln();
}

static void decode_nvrm_ioctl_bind(struct nvrm_ioctl_bind *s)
{
	print_cid(s, cid);
	print_handle(s, target, cid);
	print_handle(s, handle, cid);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_create_os_event(struct nvrm_ioctl_create_os_event *s)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_handle(s, ehandle, cid);
	print_i32(s, fd);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_destroy_os_event(struct nvrm_ioctl_destroy_os_event *s)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_i32(s, fd);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_create_simple(struct nvrm_ioctl_create_simple *s)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_handle(s, handle, cid);
	print_class(s, cls);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_get_param(struct nvrm_ioctl_get_param *s)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, key);
	print_u32(s, value);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_query_gpu_params(struct nvrm_query_gpu_params *q)
{
	print_u32(q, unk00);
	print_u32(q, unk04);
	print_u32(q, unk08);
	print_u32(q, unk0c);
	print_u32(q, unk10);
	print_u32(q, compressible_vram_size);
	print_u32(q, unk18);
	print_u32(q, unk1c);
	print_u32(q, unk20);
	print_u32(q, nv50_gpu_units);
	print_u32(q, unk28);
	print_u32(q, unk2c);
	print_ln();
}

static void decode_nvrm_query_object_classes(struct nvrm_query_object_classes *q, struct mmt_memory_dump *args, int argc)
{
	print_u32(q, cnt);
	print_pad_u32(q, _pad);
	struct mmt_buf *data = print_ptr(q, ptr, args, argc);
	print_ln();
	if (data)
		dump_mmt_buf_as_words_desc(data, demmt_nvrm_get_class_name);
}

static void decode_nvrm_query_unk019a(struct nvrm_query_unk019a *q)
{
	print_u32(q, unk00);
	print_ln();
}

#define _(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), FUN, NULL }
#define _a(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), NULL, FUN }

static struct
{
	uint32_t query;
	const char *name;
	size_t argsize;
	void *fun;
	void *fun_with_args;
} queries[] =
{
	_(NVRM_QUERY_GPU_PARAMS, struct nvrm_query_gpu_params, decode_nvrm_query_gpu_params),
	_a(NVRM_QUERY_OBJECT_CLASSES, struct nvrm_query_object_classes, decode_nvrm_query_object_classes),
	_(NVRM_QUERY_UNK019A, struct nvrm_query_unk019a, decode_nvrm_query_unk019a),
};
#undef _
#undef _a

static void decode_nvrm_ioctl_query(struct nvrm_ioctl_query *s, struct mmt_memory_dump *args, int argc)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, query);
	print_u32(s, size);
	struct mmt_buf *data = print_ptr(s, ptr, args, argc);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();

	int k, found = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	for (k = 0; k < ARRAY_SIZE(queries); ++k)
		if (queries[k].query == s->query)
		{
			if (queries[k].argsize == data->len)
			{
				mmt_log("    %s: ", queries[k].name);
				pfx = "";
				fun = queries[k].fun;
				if (fun)
					fun(data->data);
				fun_with_args = queries[k].fun_with_args;
				if (fun_with_args)
					fun_with_args(data->data, args, argc);
				found = 1;
			}
			break;
		}

	if (data && !found)
		dump_mmt_buf_as_words_horiz(data, "ptr[]:");
}

static void decode_nvrm_ioctl_create_unk34(struct nvrm_ioctl_create_unk34 *s)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_handle(s, handle, cid);
	print_cid(s, cid2);
	print_handle(s, handle2, cid2);
	print_u32(s, unk14);
	print_status(s, status);
	print_ln();
}

static void decode_nvrm_ioctl_unk38(struct nvrm_ioctl_unk38 *s, struct mmt_memory_dump *args, int argc)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, unk08);
	print_u32(s, size);
	struct mmt_buf *data = print_ptr(s, ptr, args, argc);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();

	if (data)
		dump_mmt_buf_as_words_horiz(data, "ptr[]:");
}

static void decode_nvrm_ioctl_unk41(struct nvrm_ioctl_unk41 *s, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data1, *data2, *data3;

	print_cid(s, cid);
	print_handle(s, handle1, cid);
	print_handle(s, handle2, cid);
	print_u32(s, cnt);
	data1 = print_ptr(s, ptr1, args, argc);
	data2 = print_ptr(s, ptr2, args, argc);
	data3 = print_ptr(s, ptr3, args, argc);
	print_u32(s, unk28);
	print_u32(s, unk2c);
	print_status(s, status);
	print_pad_u32(s, _pad);
	print_ln();

	if (data1)
		dump_mmt_buf_as_words_horiz(data1, "ptr1: ");
	if (data2)
		dump_mmt_buf_as_words_horiz(data2, "ptr2: ");
	if (data3)
		dump_mmt_buf_as_words_horiz(data3, "ptr3: ");
}

static void decode_nvrm_ioctl_unk48(struct nvrm_ioctl_unk48 *s)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, unk08);
	print_pad_u32(s, _pad);
	print_ln();
}

static void decode_nvrm_ioctl_unk52(struct nvrm_ioctl_unk52 *s, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data = print_ptr(s, ptr, args, argc);
	print_u32(s, unk08);
	print_status(s, status);
	print_ln();

	if (data)
		dump_mmt_buf_as_words_horiz(data, "ptr[]:");
}

static void decode_nvrm_ioctl_create_ctx(struct nvrm_ioctl_create_ctx *s)
{
	print_handle(s, handle, handle);
	print_u32(s, unk04);
	print_u32(s, unk08);
	print_ln();
}

static void decode_nvrm_ioctl_create_dev_obj(struct nvrm_ioctl_create_dev_obj *s)
{
	print_cid(s, cid);
	print_handle(s, handle, cid);
	print_u32(s, unk08);
	print_u32(s, unk0c);
	print_u32(s, ptr);
	print_u32(s, unk14);
	print_u32(s, unk18);
	print_u32(s, unk1c);
	print_ln();
}

static void decode_nvrm_ioctl_create_drv_obj(struct nvrm_ioctl_create_drv_obj *s)
{
	print_cid(s, cid);
	print_handle(s, parent, cid);
	print_handle(s, handle, cid);
	print_class(s, cls);
	print_status(s, status);
	print_ln();
}

#define _(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), FUN, NULL }
#define _a(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), NULL, FUN }
struct
{
	uint32_t id;
	const char *name;
	int size;
	void *fun;
	void *fun_with_args;
} ioctls[] =
{
		_(NVRM_IOCTL_CHECK_VERSION_STR, struct nvrm_ioctl_check_version_str, decode_nvrm_ioctl_check_version_str),
		_(NVRM_IOCTL_ENV_INFO, struct nvrm_ioctl_env_info, decode_nvrm_ioctl_env_info),
		_(NVRM_IOCTL_CARD_INFO, struct nvrm_ioctl_card_info, decode_nvrm_ioctl_card_info),
		_(NVRM_IOCTL_CARD_INFO2, struct nvrm_ioctl_card_info2, decode_nvrm_ioctl_card_info2),
		_a(NVRM_IOCTL_CREATE, struct nvrm_ioctl_create, decode_nvrm_ioctl_create),
		_a(NVRM_IOCTL_CALL, struct nvrm_ioctl_call, decode_nvrm_ioctl_call),
		_a(NVRM_IOCTL_UNK4D, struct nvrm_ioctl_unk4d, decode_nvrm_ioctl_unk4d),
		_(NVRM_IOCTL_CREATE_VSPACE, struct nvrm_ioctl_create_vspace, decode_nvrm_ioctl_create_vspace),
		_(NVRM_IOCTL_CREATE_DMA, struct nvrm_ioctl_create_dma, decode_nvrm_ioctl_create_dma),
		_(NVRM_IOCTL_HOST_MAP, struct nvrm_ioctl_host_map, decode_nvrm_ioctl_host_map),
		_(NVRM_IOCTL_DESTROY, struct nvrm_ioctl_destroy, decode_nvrm_ioctl_destroy),
		_(NVRM_IOCTL_VSPACE_MAP, struct nvrm_ioctl_vspace_map, decode_nvrm_ioctl_vspace_map),
		_(NVRM_IOCTL_HOST_UNMAP, struct nvrm_ioctl_host_unmap, decode_nvrm_ioctl_host_unmap),
		_(NVRM_IOCTL_MEMORY, struct nvrm_ioctl_memory, decode_nvrm_ioctl_memory),
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
		_a(NVRM_IOCTL_UNK41, struct nvrm_ioctl_unk41, decode_nvrm_ioctl_unk41),
		_(NVRM_IOCTL_UNK48, struct nvrm_ioctl_unk48, decode_nvrm_ioctl_unk48),
		_a(NVRM_IOCTL_UNK52, struct nvrm_ioctl_unk52, decode_nvrm_ioctl_unk52),
		_(NVRM_IOCTL_CREATE_CTX, struct nvrm_ioctl_create_ctx, decode_nvrm_ioctl_create_ctx),
		_(NVRM_IOCTL_CREATE_DEV_OBJ, struct nvrm_ioctl_create_dev_obj, decode_nvrm_ioctl_create_dev_obj),
		_(NVRM_IOCTL_CREATE_DRV_OBJ, struct nvrm_ioctl_create_drv_obj, decode_nvrm_ioctl_create_drv_obj),
};
#undef _
#undef _a

static void handle_nvrm_ioctl_create(struct nvrm_ioctl_create *s, struct mmt_memory_dump *args, int argc)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	uint32_t cid = s->cid;
	uint32_t parent = s->parent;
	uint32_t handle = s->handle;
	if (handle == 0)
	{
		struct mmt_buf *data = find_ptr(s->ptr, args, argc);
		if (!data || data->len < 4)
		{
			mmt_error("\"create cid\" without data - probably because of old mmt (before Sep 6 2014) was used%s\n", "");
			return;
		}
		cid = parent = handle = ((uint32_t *)data->data)[0];
	}

	pushbuf_add_object(handle, s->cls);
	nvrm_add_object(cid, parent, handle, s->cls);
}

static void handle_nvrm_ioctl_create_unk34(struct nvrm_ioctl_create_unk34 *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	nvrm_add_object(s->cid, s->parent, s->handle, 0xffffffff);//TODO:class
}

static void handle_nvrm_ioctl_memory(struct nvrm_ioctl_memory *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	if (s->handle)
		nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_create_simple(struct nvrm_ioctl_create_simple *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	pushbuf_add_object(s->handle, s->cls);
	nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_destroy(struct nvrm_ioctl_destroy *s)
{
	if (s->status != NVRM_STATUS_SUCCESS)
		return;

	nvrm_del_object(s->cid, s->parent, s->handle);
}

static void dump_args(struct mmt_memory_dump *args, int argc, uint64_t except_ptr)
{
	int i, j;

	for (i = 0; i < argc; ++i)
	{
		struct mmt_memory_dump *arg = &args[i];
		struct mmt_buf *data = arg->data;
		if (arg->addr == except_ptr)
			continue;
		mmt_log("        addr: 0x%016llx, size: %d, data:",
				(unsigned long long)arg->addr, data->len);
		for (j = 0; j < data->len / 4; ++j)
			mmt_log_cont(" 0x%08x", ((uint32_t *)data->data)[j]);
		for (j = data->len / 4 * 4; j < data->len; ++j)
			mmt_log_cont(" 0x%02x", data->data[j]);
		mmt_log_cont_nl();
	}
}

static void decode_nvrm_mthd_context_list_devices(struct nvrm_mthd_context_list_devices *m)
{
	int j;
	for (j = 0; j < 32; ++j)
		if (m->gpu_id[j] != 0 && m->gpu_id[j] != 0xffffffff)
			mmt_log_cont("gpu_id[%d]: 0x%08x ", j, m->gpu_id[j]);
	print_ln();
}

static void decode_nvrm_mthd_context_unk0201(struct nvrm_mthd_context_unk0201 *m)
{
	int j;
	for (j = 0; j < 32; ++j)
		if (m->gpu_id[j] != 0 && m->gpu_id[j] != 0xffffffff)
			mmt_log_cont("gpu_id[%d]: 0x%08x ", j, m->gpu_id[j]);
	print_ln();
}

static void decode_nvrm_mthd_context_enable_device(struct nvrm_mthd_context_enable_device *m)
{
	int j;
	print_u32(m, gpu_id);
	for (j = 0; j < 32; ++j)
		if (m->unk04[j] != 0)
			mmt_log_cont(", unk04[%d]: 0x%08x ", j, m->unk04[j]);
	print_ln();
}

static void decode_nvrm_mthd_context_unk0202(struct nvrm_mthd_context_unk0202 *m)
{
	print_u32(m, gpu_id);
	print_u32(m, unk04);
	print_u32(m, unk08);
	print_u32(m, unk0c);
	print_u32(m, unk10);
	print_u32(m, unk14);
	print_u32(m, unk18);
	print_u32(m, unk1c_gpu_id);
	print_u32(m, unk20);
	print_u32(m, unk24);
	print_ln();
}

static void decode_nvrm_mthd_device_unk1401(struct nvrm_mthd_device_unk1401 *m,
		struct mmt_memory_dump *args, int argc)
{
	int j;

	print_u32(m, cnt);
	print_handle(m, cid, cid);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	if (!data)
	{
		print_ln();
		return;
	}

	mmt_log_cont(" ->%s", "");
	for (j = 0; j < data->len; ++j)
		mmt_log_cont(" 0x%02x", data->data[j]);
	print_ln();
}

static void decode_nvrm_mthd_device_get_classes(struct nvrm_mthd_device_get_classes *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_pad_u32(m, _pad);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words_desc(data, demmt_nvrm_get_class_name);
}

static void decode_nvrm_mthd_device_unk0280(struct nvrm_mthd_device_unk0280 *m)
{
	print_u32(m, unk00);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_bus_id(struct nvrm_mthd_subdevice_get_bus_id *m)
{
	print_u32(m, main_id);
	print_u32(m, subsystem_id);
	print_u32(m, stepping);
	print_u32(m, real_product_id);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_chipset(struct nvrm_mthd_subdevice_get_chipset *m)
{
	print_u32(m, major);
	print_u32(m, minor);
	print_u32(m, stepping);
	print_ln();
}

#define _(V) { V, #V }
static struct
{
	uint32_t val;
	const char *name;
}
fb_params [] =
{
	_(NVRM_PARAM_SUBDEVICE_FB_BUS_WIDTH),
	_(NVRM_PARAM_SUBDEVICE_FB_UNK13),
	_(NVRM_PARAM_SUBDEVICE_FB_UNK23),
	_(NVRM_PARAM_SUBDEVICE_FB_UNK24),
	_(NVRM_PARAM_SUBDEVICE_FB_PART_COUNT),
	_(NVRM_PARAM_SUBDEVICE_FB_L2_CACHE_SIZE),
};
#undef _

static const char *fb_param_name(uint32_t v)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(fb_params); ++i)
		if (fb_params[i].val == v)
			return fb_params[i].name;
	return "";
}

static void decode_nvrm_mthd_subdevice_fb_get_params(struct nvrm_mthd_subdevice_fb_get_params *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_u32(m, unk04);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, fb_param_name);
}

static void decode_nvrm_mthd_device_unk1701(struct nvrm_mthd_device_unk1701 *m,
		struct mmt_memory_dump *args, int argc)
{
	int j;
	print_u32(m, cnt);
	print_u32(m, unk04);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	if (!data)
	{
		print_ln();
		return;
	}

	mmt_log_cont(" ->%s", "");
	for (j = 0; j < data->len; ++j)
		mmt_log_cont(" 0x%02x", data->data[j]);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_bus_info(struct nvrm_mthd_subdevice_get_bus_info *m)
{
	print_u32(m, unk00);
	print_u32(m, unk04);
	print_pad_u32(m, _pad1);
	print_i32_align(m, regs_size_mb, 4);
	print_u64(m, regs_base);
	print_pad_u32(m, _pad2);
	print_i32_align(m, fb_size_mb, 4);
	print_u64(m, fb_base);
	print_pad_u32(m, _pad3);
	print_i32_align(m, ramin_size_mb, 4);
	print_u64(m, ramin_base);
	print_u32(m, unk38);
	print_u32(m, unk3c);
	print_u64(m, unk40);
	print_u64(m, unk48);
	print_u64(m, unk50);
	print_u64(m, unk58);
	print_u64(m, unk60);
	print_u64(m, unk68);
	print_u64(m, unk70);
	print_u64(m, unk78);
	print_u64(m, unk80);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_fifo_engines(struct nvrm_mthd_subdevice_get_fifo_engines *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_pad_u32(m, _pad);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

#define _(V) { V, #V }
static struct
{
	uint32_t val;
	const char *name;
}
bus_params [] =
{
	_(NVRM_PARAM_SUBDEVICE_BUS_BUS_ID),
	_(NVRM_PARAM_SUBDEVICE_BUS_DEV_ID),
	_(NVRM_PARAM_SUBDEVICE_BUS_DOMAIN_ID),
};
#undef _

static const char *bus_param_name(uint32_t v)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(bus_params); ++i)
		if (bus_params[i].val == v)
			return bus_params[i].name;
	return "";
}

static void decode_nvrm_mthd_subdevice_bus_get_params(struct nvrm_mthd_subdevice_bus_get_params *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_pad_u32(m, _pad);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, bus_param_name);
}

static void decode_nvrm_mthd_device_unk1102(struct nvrm_mthd_device_unk1102 *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_pad_u32(m, _pad);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_unk0101(struct nvrm_mthd_subdevice_unk0101 *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_pad_u32(m, _ptr);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, NULL);
}

static void decode_nvrm_mthd_subdevice_unk0119(struct nvrm_mthd_subdevice_unk0119 *m)
{
	print_u32(m, unk00);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_unk1201(struct nvrm_mthd_subdevice_unk1201 *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, cnt);
	print_pad_u32(m, _pad);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_word_pairs(data, NULL);
}

static void decode_nvrm_mthd_subdevice_get_gpc_mask(struct nvrm_mthd_subdevice_get_gpc_mask *m)
{
	print_u32(m, gpc_mask);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_gpc_tp_mask(struct nvrm_mthd_subdevice_get_gpc_tp_mask *m)
{
	print_u32(m, gpc_id);
	print_u32(m, tp_mask);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_fifo_joinable_engines(struct nvrm_mthd_subdevice_get_fifo_joinable_engines *m)
{
	int j;

	print_u32(m, eng);
	print_class(m, cls);
	print_u32(m, cnt);
	print_ln();

	for (j = 0; j < 0x20; ++j)
		if (j < m->cnt || m->res[j])
			mmt_log("     [%2d]: 0x%08x\n", j, m->res[j]);
}

static void decode_nvrm_mthd_subdevice_get_fifo_classes(struct nvrm_mthd_subdevice_get_fifo_classes *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, eng);
	print_u32(m, cnt);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_get_name(struct nvrm_mthd_subdevice_get_name *m)
{
	print_u32(m, unk00);
	print_str(m, name);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_uuid(struct nvrm_mthd_subdevice_get_uuid *m)
{
	int j;
	print_u32(m, unk00);
	print_u32(m, unk04);
	print_i32(m, uuid_len);
	mmt_log_cont(", uuid: %s", "");

	for (j = 0; j < m->uuid_len; ++j)
		mmt_log_cont("%02x", (unsigned char)m->uuid[j]);

	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_compute_mode(struct nvrm_mthd_subdevice_get_compute_mode *m)
{
	print_u32(m, mode);
	print_ln();
}

static void decode_nvrm_mthd_fifo_ib_activate(struct nvrm_mthd_fifo_ib_activate *m)
{
	print_u32(m, unk00);
	print_ln();
}

static void decode_nvrm_mthd_context_unk0301(struct nvrm_mthd_context_unk0301 *m)
{
	int j;
	print_ln();

	for (j = 0; j < 12; ++j)
		mmt_log("        0x%08x\n", m->unk00[j]);
}

static void decode_nvrm_mthd_context_disable_device(struct nvrm_mthd_context_disable_device *m)
{
	int j;
	print_u32(m, gpu_id);
	print_ln();

	for (j = 0; j < 31; ++j)
		mmt_log("        0x%08x\n", m->unk04[j]);
}

static void decode_nvrm_mthd_device_unk170d(struct nvrm_mthd_device_unk170d *m,
		struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data1, *data2;

	print_u32(m, unk00);
	print_u32(m, unk04);
	data1 = print_ptr(m, ptr, args, argc);
	data2 = print_ptr(m, unk10, args, argc);
	print_ln();

	if (data1)
		dump_mmt_buf_as_words(data1);

	if (data2)
	{
		mmt_log("%s\n", "");

		dump_mmt_buf_as_words(data2);
	}
}

static void decode_nvrm_mthd_subdevice_bar0(struct nvrm_mthd_subdevice_bar0 *m,
		struct mmt_memory_dump *args, int argc)
{
	print_handle(m, cid, cid);
	print_handle(m, handle, cid);
	print_u32(m, unk08);
	print_u32(m, unk0c);
	print_u32(m, unk10);
	print_i32(m, cnt);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_get_gpu_id(struct nvrm_mthd_subdevice_get_gpu_id *m)
{
	print_u32(m, gpu_id);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_get_time(struct nvrm_mthd_subdevice_get_time *m)
{
	print_u64(m, time);
	print_ln();
}

static void decode_nvrm_mthd_subdevice_unk0512(struct nvrm_mthd_subdevice_unk0512 *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, unk00);
	print_u32(m, unk04);
	print_u32(m, size);
	print_u32(m, unk0c);
	print_u32(m, unk10);
	print_u32(m, unk14);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_unk0522(struct nvrm_mthd_subdevice_unk0522 *m,
		struct mmt_memory_dump *args, int argc)
{
	print_u32(m, unk00);
	print_u32(m, unk04);
	print_u32(m, size);
	print_u32(m, unk0c);
	print_u32(m, unk10);
	print_u32(m, unk14);
	struct mmt_buf *data = print_ptr(m, ptr, args, argc);
	print_ln();

	if (data)
		dump_mmt_buf_as_words(data);
}

static void decode_nvrm_mthd_subdevice_unk200a(struct nvrm_mthd_subdevice_unk200a *m)
{
	print_u32(m, unk00);
	print_u32(m, unk04);
	print_ln();
}

static void decode_nvrm_mthd_fifo_ib_object_info(struct nvrm_mthd_fifo_ib_object_info *m)
{
	print_u32(m, handle);
	print_u32(m, name);
	pfx = ", hw"; print_class(m, hwcls);
	print_u32(m, eng);
	print_ln();
}

static void decode_nvrm_mthd_context_unk021b(struct nvrm_mthd_context_unk021b *m)
{
	print_u32(m, gpu_id);
	print_u32(m, unk04);
	print_u32(m, unk08);
	print_ln();
}

#define _(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), FUN, NULL }
#define _a(MTHD, STR, FUN) { MTHD, #MTHD , sizeof(STR), NULL, FUN }

static struct
{
	uint32_t mthd;
	const char *name;
	size_t argsize;
	void *fun;
	void *fun_with_args;
} mthds[] =
{
	_(NVRM_MTHD_CONTEXT_LIST_DEVICES, struct nvrm_mthd_context_list_devices, decode_nvrm_mthd_context_list_devices),
	_(NVRM_MTHD_CONTEXT_ENABLE_DEVICE, struct nvrm_mthd_context_enable_device, decode_nvrm_mthd_context_enable_device),
	_(NVRM_MTHD_CONTEXT_UNK0202, struct nvrm_mthd_context_unk0202, decode_nvrm_mthd_context_unk0202),
	_(NVRM_MTHD_CONTEXT_UNK0201, struct nvrm_mthd_context_unk0201, decode_nvrm_mthd_context_unk0201),
	_a(NVRM_MTHD_DEVICE_UNK1401, struct nvrm_mthd_device_unk1401, decode_nvrm_mthd_device_unk1401),
	_a(NVRM_MTHD_DEVICE_GET_CLASSES, struct nvrm_mthd_device_get_classes, decode_nvrm_mthd_device_get_classes),
	_(NVRM_MTHD_DEVICE_UNK0280, struct nvrm_mthd_device_unk0280, decode_nvrm_mthd_device_unk0280),
	_(NVRM_MTHD_SUBDEVICE_GET_BUS_ID, struct nvrm_mthd_subdevice_get_bus_id, decode_nvrm_mthd_subdevice_get_bus_id),
	_(NVRM_MTHD_SUBDEVICE_GET_CHIPSET, struct nvrm_mthd_subdevice_get_chipset, decode_nvrm_mthd_subdevice_get_chipset),
	_a(NVRM_MTHD_SUBDEVICE_FB_GET_PARAMS, struct nvrm_mthd_subdevice_fb_get_params, decode_nvrm_mthd_subdevice_fb_get_params),
	_a(NVRM_MTHD_DEVICE_UNK1701, struct nvrm_mthd_device_unk1701, decode_nvrm_mthd_device_unk1701),
	_(NVRM_MTHD_SUBDEVICE_GET_BUS_INFO, struct nvrm_mthd_subdevice_get_bus_info, decode_nvrm_mthd_subdevice_get_bus_info),
	_a(NVRM_MTHD_SUBDEVICE_GET_FIFO_ENGINES, struct nvrm_mthd_subdevice_get_fifo_engines, decode_nvrm_mthd_subdevice_get_fifo_engines),
	_a(NVRM_MTHD_SUBDEVICE_BUS_GET_PARAMS, struct nvrm_mthd_subdevice_bus_get_params, decode_nvrm_mthd_subdevice_bus_get_params),
	_a(NVRM_MTHD_DEVICE_UNK1102, struct nvrm_mthd_device_unk1102, decode_nvrm_mthd_device_unk1102),
	_a(NVRM_MTHD_SUBDEVICE_UNK0101, struct nvrm_mthd_subdevice_unk0101, decode_nvrm_mthd_subdevice_unk0101),
	_(NVRM_MTHD_SUBDEVICE_UNK0119, struct nvrm_mthd_subdevice_unk0119, decode_nvrm_mthd_subdevice_unk0119),
	_a(NVRM_MTHD_SUBDEVICE_UNK1201, struct nvrm_mthd_subdevice_unk1201, decode_nvrm_mthd_subdevice_unk1201),
	_(NVRM_MTHD_SUBDEVICE_GET_GPC_MASK, struct nvrm_mthd_subdevice_get_gpc_mask, decode_nvrm_mthd_subdevice_get_gpc_mask),
	_(NVRM_MTHD_SUBDEVICE_GET_GPC_TP_MASK, struct nvrm_mthd_subdevice_get_gpc_tp_mask, decode_nvrm_mthd_subdevice_get_gpc_tp_mask),
	_(NVRM_MTHD_SUBDEVICE_GET_FIFO_JOINABLE_ENGINES, struct nvrm_mthd_subdevice_get_fifo_joinable_engines, decode_nvrm_mthd_subdevice_get_fifo_joinable_engines),
	_a(NVRM_MTHD_SUBDEVICE_GET_FIFO_CLASSES, struct nvrm_mthd_subdevice_get_fifo_classes, decode_nvrm_mthd_subdevice_get_fifo_classes),
	_(NVRM_MTHD_SUBDEVICE_GET_NAME, struct nvrm_mthd_subdevice_get_name, decode_nvrm_mthd_subdevice_get_name),
	_(NVRM_MTHD_SUBDEVICE_GET_UUID, struct nvrm_mthd_subdevice_get_uuid, decode_nvrm_mthd_subdevice_get_uuid),
	_(NVRM_MTHD_SUBDEVICE_GET_COMPUTE_MODE, struct nvrm_mthd_subdevice_get_compute_mode, decode_nvrm_mthd_subdevice_get_compute_mode),
	_(NVRM_MTHD_FIFO_IB_ACTIVATE, struct nvrm_mthd_fifo_ib_activate, decode_nvrm_mthd_fifo_ib_activate),
	_(NVRM_MTHD_CONTEXT_UNK0301, struct nvrm_mthd_context_unk0301, decode_nvrm_mthd_context_unk0301),
	_(NVRM_MTHD_CONTEXT_DISABLE_DEVICE, struct nvrm_mthd_context_disable_device, decode_nvrm_mthd_context_disable_device),
	_a(NVRM_MTHD_DEVICE_UNK170D, struct nvrm_mthd_device_unk170d, decode_nvrm_mthd_device_unk170d),
	_a(NVRM_MTHD_SUBDEVICE_BAR0, struct nvrm_mthd_subdevice_bar0, decode_nvrm_mthd_subdevice_bar0),
	_(NVRM_MTHD_SUBDEVICE_GET_GPU_ID, struct nvrm_mthd_subdevice_get_gpu_id, decode_nvrm_mthd_subdevice_get_gpu_id),
	_(NVRM_MTHD_SUBDEVICE_GET_TIME, struct nvrm_mthd_subdevice_get_time, decode_nvrm_mthd_subdevice_get_time),
	_a(NVRM_MTHD_SUBDEVICE_UNK0512, struct nvrm_mthd_subdevice_unk0512, decode_nvrm_mthd_subdevice_unk0512),
	_a(NVRM_MTHD_SUBDEVICE_UNK0522, struct nvrm_mthd_subdevice_unk0522, decode_nvrm_mthd_subdevice_unk0522),
	_(NVRM_MTHD_SUBDEVICE_UNK200A, struct nvrm_mthd_subdevice_unk200a, decode_nvrm_mthd_subdevice_unk200a),
	_(NVRM_MTHD_FIFO_IB_OBJECT_INFO, struct nvrm_mthd_fifo_ib_object_info, decode_nvrm_mthd_fifo_ib_object_info),
	_(NVRM_MTHD_FIFO_IB_OBJECT_INFO2, struct nvrm_mthd_fifo_ib_object_info, decode_nvrm_mthd_fifo_ib_object_info),
	_(NVRM_MTHD_CONTEXT_UNK021B, struct nvrm_mthd_context_unk021b, decode_nvrm_mthd_context_unk021b),
};
#undef _
#undef _a

static void handle_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc, int pre)
{
	struct mmt_buf *data = find_ptr(s->ptr, args, argc);
	if (!data)
	{
		if (!dump_ioctls)
			dump_args(args, argc, 0);
		return;
	}

	int k, found = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	for (k = 0; k < ARRAY_SIZE(mthds); ++k)
		if (mthds[k].mthd == s->mthd)
		{
			if (mthds[k].argsize == data->len)
			{
				mmt_log("    %s: ", mthds[k].name);
				pfx = "";
				fun = mthds[k].fun;
				if (fun)
					fun(data->data);
				fun_with_args = mthds[k].fun_with_args;
				if (fun_with_args)
					fun_with_args(data->data, args, argc);
				found = 1;
			}
			break;
		}

	if (found && (s->mthd == NVRM_MTHD_FIFO_IB_OBJECT_INFO || s->mthd == NVRM_MTHD_FIFO_IB_OBJECT_INFO2) && !pre)
	{
		struct nvrm_mthd_fifo_ib_object_info *s = (void *) data->data;
		pushbuf_add_object_name(s->handle, s->name);
	}

	if (!dump_ioctls)
	{
		if (!found)
			dump_args(args, argc, 0);
		else if (argc != 1 && fun_with_args == NULL)
			dump_args(args, argc, s->ptr);
	}
}

static void handle_nvrm_ioctl_create_dma(struct nvrm_ioctl_create_dma *s)
{
	nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_create_vspace(struct nvrm_ioctl_create_vspace *s)
{
	if (s->foffset != 0)
	{
		struct buffer *buf;
		int found = 0;
		for (buf = buffers_list; buf != NULL; buf = buf->next)
			if (buf->mmap_offset == s->foffset)
			{
				buf->data1 = s->parent;
				buf->data2 = s->handle;

				found = 1;
				break;
			}

		if (!found)
		{
			for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
			{
				if (buf->data1 == s->parent && buf->data2 == s->handle)
				{
					mmt_log("TODO: gpu only buffer found (0x%016lx), NVRM_IOCTL_CREATE_VSPACE handling needs to be updated!\n", buf->gpu_start);
					break;
				}
			}

			struct unk_map *m = malloc(sizeof(struct unk_map));
			m->data1 = s->parent;
			m->data2 = s->handle;
			m->mmap_offset = s->foffset;
			m->next = unk_maps;
			unk_maps = m;
		}
	}

	nvrm_add_object(s->cid, s->parent, s->handle, s->cls);
}

static void handle_nvrm_ioctl_host_map(struct nvrm_ioctl_host_map *s)
{
	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->mmap_offset == s->foffset)
		{
			buf->data1 = s->subdev;
			buf->data2 = s->handle;

			found = 1;
			break;
		}

	if (!found)
	{
		for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		{
			if (buf->data1 == s->subdev && buf->data2 == s->handle)
			{
				if (buf->mmap_offset != s->foffset)
				{
					mmt_debug("gpu only buffer found (0x%016lx), merging mmap_offset\n", buf->gpu_start);
					buf->mmap_offset = s->foffset;
				}

				found = 1;
				break;
			}
		}

		if (!found)
		{
			struct unk_map *m = malloc(sizeof(struct unk_map));
			m->data1 = s->subdev;
			m->data2 = s->handle;
			m->mmap_offset = s->foffset;
			m->next = unk_maps;
			unk_maps = m;
		}
	}
}

static void handle_nvrm_ioctl_vspace_map(struct nvrm_ioctl_vspace_map *s)
{
	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == s->dev && buf->data2 == s->handle && buf->length == s->size)
		{
			buf->gpu_start = s->addr;
			mmt_debug("setting gpu address for buffer %d to 0x%08lx\n", buf->id, buf->gpu_start);

			found = 1;
			break;
		}

	if (!found)
	{
		struct unk_map *tmp;
		for (tmp = unk_maps; tmp != NULL; tmp = tmp->next)
		{
			if (tmp->data1 == s->dev && tmp->data2 == s->handle)
			{
				mmt_log("TODO: unk buffer found, demmt_nv_gpu_map needs to be updated!%s\n", "");
				break;
			}
		}

		register_gpu_only_buffer(s->addr, s->size, 0, s->dev, s->handle);
	}
}

static void handle_nvrm_ioctl_vspace_unmap(struct nvrm_ioctl_vspace_unmap *s)
{
	struct buffer *buf;
	int found = 0;
	for (buf = buffers_list; buf != NULL; buf = buf->next)
		if (buf->data1 == s->dev && buf->data2 == s->handle &&
				buf->gpu_start == s->addr)
		{
			mmt_debug("clearing gpu address for buffer %d (was: 0x%08lx)\n", buf->id, buf->gpu_start);
			buf->gpu_start = 0;

			found = 1;
			break;
		}

	if (!found)
	{
		for (buf = gpu_only_buffers_list; buf != NULL; buf = buf->next)
		{
			if (buf->data1 == s->dev && buf->data2 == s->handle && buf->gpu_start == s->addr)
			{
				mmt_debug("deregistering gpu only buffer of size %ld\n", buf->length);
				buffer_free(buf);

				found = 1;
				break;
			}
		}

		if (!found)
			mmt_log("gpu only buffer not found%s\n", "");
	}
}

static void handle_nvrm_ioctl_host_unmap(struct nvrm_ioctl_host_unmap *s)
{
}

int demmt_nv_ioctl_pre(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	int k, found = 0;
	int args_used = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	for (k = 0; k < ARRAY_SIZE(ioctls); ++k)
		if (ioctls[k].id == id)
		{
			if (ioctls[k].size == buf->len)
			{
				mmt_log("%-26s pre,  ", ioctls[k].name);
				pfx = "";
				fun = ioctls[k].fun;
				if (fun)
					fun(buf->data);
				fun_with_args = ioctls[k].fun_with_args;
				if (fun_with_args)
				{
					fun_with_args(buf->data, args, argc);
					args_used = 1;
				}
				found = 1;
			}
			break;
		}

	if (!found)
		return 1;

	void *d = buf->data;

	if (id == NVRM_IOCTL_CALL)
	{
		handle_nvrm_ioctl_call(d, args, argc, 1);
		args_used = 1;
	}

	if (!args_used || dump_ioctls)
		dump_args(args, argc, 0);

	return 0;
}

int demmt_nv_ioctl_post(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	int k, found = 0;
	int args_used = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	for (k = 0; k < ARRAY_SIZE(ioctls); ++k)
		if (ioctls[k].id == id)
		{
			if (ioctls[k].size == buf->len)
			{
				mmt_log("%-26s post, ", ioctls[k].name);
				pfx = "";
				fun = ioctls[k].fun;
				if (fun)
					fun(buf->data);
				fun_with_args = ioctls[k].fun_with_args;
				if (fun_with_args)
				{
					fun_with_args(buf->data, args, argc);
					args_used = 1;
				}
				found = 1;
			}
			break;
		}

	if (!found)
		return 1;

	void *d = buf->data;

	if (id == NVRM_IOCTL_CREATE)
		handle_nvrm_ioctl_create(d, args, argc);
	else if (id == NVRM_IOCTL_CREATE_SIMPLE)
		handle_nvrm_ioctl_create_simple(d);
	else if (id == NVRM_IOCTL_DESTROY)
		handle_nvrm_ioctl_destroy(d);
	else if (id == NVRM_IOCTL_CREATE_VSPACE)
		handle_nvrm_ioctl_create_vspace(d);
	else if (id == NVRM_IOCTL_HOST_MAP)
		handle_nvrm_ioctl_host_map(d);
	else if (id == NVRM_IOCTL_VSPACE_MAP)
		handle_nvrm_ioctl_vspace_map(d);
	else if (id == NVRM_IOCTL_VSPACE_UNMAP)
		handle_nvrm_ioctl_vspace_unmap(d);
	else if (id == NVRM_IOCTL_HOST_UNMAP)
		handle_nvrm_ioctl_host_unmap(d);
	else if (id == NVRM_IOCTL_CALL)
	{
		handle_nvrm_ioctl_call(d, args, argc, 0);
		args_used = 1;
	}
	else if (id == NVRM_IOCTL_CREATE_DMA)
		handle_nvrm_ioctl_create_dma(d);
	else if (id == NVRM_IOCTL_CREATE_UNK34)
		handle_nvrm_ioctl_create_unk34(d);
	else if (id == NVRM_IOCTL_MEMORY)
		handle_nvrm_ioctl_memory(d);

	if (!args_used || dump_ioctls)
		dump_args(args, argc, 0);

	return 0;
}

void demmt_memory_dump(struct mmt_memory_dump_prefix *d, struct mmt_buf *b, void *state)
{
	// dead code, because memory dumps are passed to ioctl_pre / ioctl_post handlers
	int i;
	mmt_log("memory dump, addr: 0x%016lx, txt: \"%s\", data.len: %d, data:", d->addr, d->str.data, b->len);

	for (i = 0; i < b->len / 4; ++i)
		mmt_log_cont(" 0x%08x", ((uint32_t *)b->data)[i]);
	mmt_log_cont("%s", "\n");
}

void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state)
{
	mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
			(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);
	__demmt_mmap(mm->id, mm->start, mm->len, mm->offset, &mm->data1, &mm->data2);
}

void demmt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state)
{
}

void demmt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state)
{
	mmt_log("ioctl4d: %s\n", ctl->str.data);
}

void demmt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state)
{
}
