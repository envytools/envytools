/*
 * Copyright (C) 2015 Marcin Ślusarz <marcin.slusarz@gmail.com>.
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

#include <sys/ioctl.h>
#include <stdlib.h>

#include "config.h"
#include "decode_utils.h"
#include "fglrx.h"
#include "fglrx_ioctl.h"
#include "log.h"
#include "util.h"

int fglrx_show_unk_zero_fields = 1;

int _fglrx_field_enabled(const char *name)
{
	if (!fglrx_show_unk_zero_fields && strncmp(name, "unk", 3) == 0)
		return 0;
	return 1;
}

#define fglrx_field_enabled(strct, field) ((strct)->field || _fglrx_field_enabled(#field))

static const char *fglrx_sep = ", ";
static const char *fglrx_pfx = "";

static inline void fglrx_reset_pfx()
{
	fglrx_pfx = "";
}

#define fglrx_print_x64(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_x64(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_x32(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_x32(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_x16(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_x16(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_x8(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_x8( fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)

#define fglrx_print_d64_align(strct, field, algn)	do { if (fglrx_field_enabled(strct, field)) _print_d32_align(fglrx_pfx, strct, field, #algn); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_d32_align(strct, field, algn)	do { if (fglrx_field_enabled(strct, field)) _print_d32_align(fglrx_pfx, strct, field, #algn); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_d16_align(strct, field, algn)	do { if (fglrx_field_enabled(strct, field)) _print_d16_align(fglrx_pfx, strct, field, #algn); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_d8_align(strct, field, algn)	do { if (fglrx_field_enabled(strct, field)) _print_d8_align( fglrx_pfx, strct, field, #algn); fglrx_pfx = fglrx_sep; } while (0)

#define fglrx_print_d64(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_d64(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_d32(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_d32(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_d16(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_d16(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)
#define fglrx_print_d8(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_d8( fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)

#define fglrx_print_pad_x32(strct, field)			do { if ((strct)->field) mmt_log_cont("%s%s" #field ": 0x%08x%s",   fglrx_pfx, colors->err, (strct)->field, colors->reset); fglrx_pfx = fglrx_sep; } while (0)

#define fglrx_print_str(strct, field)				do { if (fglrx_field_enabled(strct, field)) _print_str(fglrx_pfx, strct, field); fglrx_pfx = fglrx_sep; } while (0)

#define fglrx_print_ptr(strct, field, args, argc) \
	({ \
		struct mmt_buf *__ret = NULL; \
		if (fglrx_field_enabled(strct, field)) { \
			if (strct->field) { \
				mmt_log_cont("%s" #field ": 0x%016" PRIx64 "", fglrx_pfx, strct->field); \
				if (argc > 0) \
					__ret = find_ptr(strct->field, args, argc); \
				if (__ret == NULL) \
					mmt_log_cont("%s", " [no data]"); \
			} else \
				mmt_log_cont("%s" #field ": %s", fglrx_pfx, "NULL"); \
		} \
		fglrx_pfx = fglrx_sep; \
		__ret; \
	})

static void decode_fglrx_ioctl_drv_info(struct fglrx_ioctl_drv_info *d, struct mmt_memory_dump *args, int argc)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_x32(d, unk08);
	fglrx_print_x32(d, unk0c);
	fglrx_print_d32(d, drv_id_len);
	fglrx_print_pad_x32(d, _pad14);
	struct mmt_buf *drv_id = fglrx_print_ptr(d, drv_id, args, argc);
	fglrx_print_d32(d, drv_date_len);
	fglrx_print_pad_x32(d, _pad24);
	struct mmt_buf *drv_date = fglrx_print_ptr(d, drv_date, args, argc);
	fglrx_print_d32(d, drv_name_len);
	fglrx_print_pad_x32(d, _pad34);
	struct mmt_buf *drv_name = fglrx_print_ptr(d, drv_name, args, argc);
	mmt_log_cont_nl();

	if (drv_id)
		dump_mmt_buf_as_text(drv_id, "drv_id");
	if (drv_date)
		dump_mmt_buf_as_text(drv_date, "drv_date");
	if (drv_name)
		dump_mmt_buf_as_text(drv_name, "drv_name");
}

static void decode_fglrx_ioctl_get_bus_id(struct fglrx_ioctl_get_bus_id *d, struct mmt_memory_dump *args, int argc)
{
	fglrx_print_d32(d, bus_id_len);
	fglrx_print_pad_x32(d, _pad04);
	struct mmt_buf *data1 = fglrx_print_ptr(d, bus_id, args, argc);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_text(data1, "bus_id");
}

static void decode_fglrx_ioctl_02(struct fglrx_ioctl_02 *d)
{
	fglrx_print_x32(d, unk00);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_07(struct fglrx_ioctl_07 *d)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_x32(d, unk08);
	fglrx_print_x32(d, unk0c);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_20(struct fglrx_ioctl_20 *d)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_2a(struct fglrx_ioctl_2a *d)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_2b(struct fglrx_ioctl_2b *d)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_46(struct fglrx_ioctl_46 *d)
{
	fglrx_print_x32(d, unk00);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_47(struct fglrx_ioctl_47 *d)
{
	fglrx_print_x32(d, unk00);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_4f(struct fglrx_ioctl_4f *d, struct mmt_memory_dump *args, int argc)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_x32(d, unk08);
	fglrx_print_x32(d, unk0c);
	fglrx_print_x32(d, unk10);
	fglrx_print_x32(d, unk14);
	fglrx_print_x32(d, unk18);
	fglrx_print_x32(d, unk1c);
	struct mmt_buf *data1 = fglrx_print_ptr(d, ptr1, args, argc);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_words_horiz(data1, "ptr1[]:");
}

static void decode_fglrx_ioctl_50(struct fglrx_ioctl_50 *d, struct mmt_memory_dump *args, int argc)
{
	fglrx_print_d32(d, kernel_ver_len);
	fglrx_print_pad_x32(d, _pad04);
	struct mmt_buf *data1 = fglrx_print_ptr(d, kernel_ver, args, argc);
	fglrx_print_x32(d, unk10);
	fglrx_print_x32(d, unk14);
	fglrx_print_x32(d, unk18);
	fglrx_print_x32(d, unk1c);
	fglrx_print_x32(d, unk20);
	fglrx_print_x32(d, unk24);
	fglrx_print_x32(d, unk28);
	fglrx_print_x32(d, unk2c);
	fglrx_print_x32(d, unk30);
	fglrx_print_x32(d, unk34);
	fglrx_print_x32(d, unk38);
	fglrx_print_x32(d, unk3c);
	fglrx_print_x32(d, unk40);
	fglrx_print_x32(d, unk44);
	fglrx_print_x32(d, unk48);
	fglrx_print_x32(d, unk4c);
	fglrx_print_x32(d, unk50);
	fglrx_print_x32(d, unk54);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_text(data1, "kernel_ver");
}

static void decode_fglrx_ioctl_54(struct fglrx_ioctl_54 *d)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_x32(d, unk08);
	fglrx_print_x32(d, unk0c);
	fglrx_print_x32(d, unk10);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_57(struct fglrx_ioctl_57 *d)
{
	fglrx_print_x32(d, unk00);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_58(struct fglrx_ioctl_58 *d)
{
	fglrx_print_x32(d, unk00);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_64(struct fglrx_ioctl_64 *d, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data1 = fglrx_print_ptr(d, ptr1, args, argc);
	fglrx_print_x32(d, unk08);
	fglrx_print_x32(d, unk0c);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_words_horiz(data1, "ptr1[]:");
}

static void decode_fglrx_ioctl_68(struct fglrx_ioctl_68 *d, struct mmt_memory_dump *args, int argc)
{
	struct mmt_buf *data1 = fglrx_print_ptr(d, ptr1, args, argc);
	struct mmt_buf *data2 = fglrx_print_ptr(d, ptr2, args, argc);
	fglrx_print_x32(d, unk10);
	fglrx_print_x32(d, unk14);
	fglrx_print_x32(d, unk18);
	fglrx_print_x32(d, unk1c);
	fglrx_print_x32(d, unk20);
	fglrx_print_x32(d, unk24);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_words_horiz(data1, "ptr1[]:");
	if (data2)
		dump_mmt_buf_as_words_horiz(data2, "ptr2[]:");
}

static void decode_fglrx_ioctl_69(struct fglrx_ioctl_69 *d)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_x32(d, unk08);
	fglrx_print_x32(d, unk0c);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_config(struct fglrx_ioctl_config *d, struct mmt_memory_dump *args, int argc)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_x32(d, unk08);
	fglrx_print_d32(d, username_len);
	struct mmt_buf *data1 = fglrx_print_ptr(d, username, args, argc);
	fglrx_print_d32(d, namespace_len);
	fglrx_print_pad_x32(d, _pad1c);
	struct mmt_buf *data2 = fglrx_print_ptr(d, namespace, args, argc);
	fglrx_print_d32(d, prop_name_len);
	fglrx_print_pad_x32(d, _pad2c);
	struct mmt_buf *data3 = fglrx_print_ptr(d, prop_name, args, argc);
	fglrx_print_x32(d, unk38);
	fglrx_print_x32(d, unk3c);
	fglrx_print_x32(d, unk40);
	fglrx_print_x32(d, unk44);
	fglrx_print_x32(d, unk48);
	fglrx_print_x32(d, unk4c);
	fglrx_print_d32(d, prop_value_len);
	fglrx_print_pad_x32(d, _pad54);
	struct mmt_buf *data4 = fglrx_print_ptr(d, prop_value, args, argc);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_text(data1, "username");
	if (data2)
		dump_mmt_buf_as_text(data2, "namespace");
	if (data3)
		dump_mmt_buf_as_text(data3, "prop_name");
	if (data4)
		dump_mmt_buf_as_text(data4, "prop_value");
}

static void decode_fglrx_ioctl_84(struct fglrx_ioctl_84 *d)
{
	fglrx_print_x32(d, unk00);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_8b(struct fglrx_ioctl_8b *d)
{
	int i;
	for (i = 0; i < 129; ++i)
		fglrx_print_x32(d, unk[i]);
	mmt_log_cont_nl();
}

static void decode_fglrx_ioctl_a6(struct fglrx_ioctl_a6 *d, struct mmt_memory_dump *args, int argc)
{
	fglrx_print_x32(d, unk00);
	fglrx_print_x32(d, unk04);
	fglrx_print_d32(d, len1);
	fglrx_print_pad_x32(d, _pad0c);
	struct mmt_buf *data1 = fglrx_print_ptr(d, ptr1, args, argc);
	fglrx_print_d32(d, len2);
	fglrx_print_pad_x32(d, _pad1c);
	struct mmt_buf *data2 = fglrx_print_ptr(d, ptr2, args, argc);
	fglrx_print_x32(d, unk28);
	fglrx_print_x32(d, unk2c);
	mmt_log_cont_nl();

	if (data1)
		dump_mmt_buf_as_words_horiz(data1, "ptr1[]:");
	if (data2)
		dump_mmt_buf_as_words_horiz(data2, "ptr2[]:");
}

struct fglrx_ioctl
{
	uint32_t id;
	const char *name;
	int size;
	void *fun;
	void *fun_with_args;
	int disabled;
};

#define _(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), FUN, NULL, 0 }
#define _a(CTL, STR, FUN) { CTL, #CTL , sizeof(STR), NULL, FUN, 0 }
static struct fglrx_ioctl fglrx_ioctls[] =
{
		_a(FGLRX_IOCTL_DRV_INFO, struct fglrx_ioctl_drv_info, decode_fglrx_ioctl_drv_info),
		_a(FGLRX_IOCTL_GET_BUS_ID, struct fglrx_ioctl_get_bus_id, decode_fglrx_ioctl_get_bus_id),
		_(FGLRX_IOCTL_02, struct fglrx_ioctl_02, decode_fglrx_ioctl_02),
		_(FGLRX_IOCTL_07, struct fglrx_ioctl_07, decode_fglrx_ioctl_07),
		_(FGLRX_IOCTL_20, struct fglrx_ioctl_20, decode_fglrx_ioctl_20),
		_(FGLRX_IOCTL_2A, struct fglrx_ioctl_2a, decode_fglrx_ioctl_2a),
		_(FGLRX_IOCTL_2B, struct fglrx_ioctl_2b, decode_fglrx_ioctl_2b),
		_(FGLRX_IOCTL_46, struct fglrx_ioctl_46, decode_fglrx_ioctl_46),
		_(FGLRX_IOCTL_47, struct fglrx_ioctl_47, decode_fglrx_ioctl_47),
		_a(FGLRX_IOCTL_4F, struct fglrx_ioctl_4f, decode_fglrx_ioctl_4f),
		_a(FGLRX_IOCTL_50, struct fglrx_ioctl_50, decode_fglrx_ioctl_50),
		_(FGLRX_IOCTL_54, struct fglrx_ioctl_54, decode_fglrx_ioctl_54),
		_(FGLRX_IOCTL_57, struct fglrx_ioctl_57, decode_fglrx_ioctl_57),
		_(FGLRX_IOCTL_58, struct fglrx_ioctl_58, decode_fglrx_ioctl_58),
		_a(FGLRX_IOCTL_64, struct fglrx_ioctl_64, decode_fglrx_ioctl_64),
		_a(FGLRX_IOCTL_68, struct fglrx_ioctl_68, decode_fglrx_ioctl_68),
		_(FGLRX_IOCTL_69, struct fglrx_ioctl_69, decode_fglrx_ioctl_69),
		_a(FGLRX_IOCTL_CONFIG, struct fglrx_ioctl_config, decode_fglrx_ioctl_config),
		_(FGLRX_IOCTL_84, struct fglrx_ioctl_84, decode_fglrx_ioctl_84),
		_(FGLRX_IOCTL_8b, struct fglrx_ioctl_8b, decode_fglrx_ioctl_8b),
		_a(FGLRX_IOCTL_A6, struct fglrx_ioctl_a6, decode_fglrx_ioctl_a6),
};
#undef _
#undef _a
static int fglrx_ioctls_cnt = ARRAY_SIZE(fglrx_ioctls);

static int decode_fglrx_ioctl(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr,
		uint16_t size, struct mmt_buf *buf, uint64_t ret, uint64_t err,
		void *state, struct mmt_memory_dump *args, int argc, const char *name)
{
	int k, found = 0;
	int args_used = 0;
	void (*fun)(void *) = NULL;
	void (*fun_with_args)(void *, struct mmt_memory_dump *, int argc) = NULL;

	struct fglrx_ioctl *ioctl;
	for (k = 0; k < fglrx_ioctls_cnt; ++k)
	{
		ioctl = &fglrx_ioctls[k];
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

					fglrx_reset_pfx();
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


int fglrx_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc)
{
	return decode_fglrx_ioctl(fd, id, dir, nr, size, buf, 0, 0, state, args, argc, "pre,");
}

int fglrx_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc)
{
	return decode_fglrx_ioctl(fd, id, dir, nr, size, buf, ret, err, state, args, argc, "post,");
}
