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

#include "decode_utils.h"
#include "log.h"

void dump_mmt_buf_as_text(struct mmt_buf *buf, const char *pfx)
{
	mmt_log("        %s: %.*s\n", pfx, buf->len, buf->data);
}

void dump_mmt_buf_as_words(struct mmt_buf *buf)
{
	int j;
	for (j = 0; j < buf->len / 4; ++j)
		mmt_log("        0x%08x\n", ((uint32_t *)buf->data)[j]);
	for (j = buf->len / 4 * 4; j < buf->len; ++j)
		mmt_log("        0x%02x\n", buf->data[j]);
}

void dump_mmt_buf_as_words_horiz(struct mmt_buf *buf, const char *pfx)
{
	int j;
	mmt_log("        %s", pfx ? pfx : "");
	for (j = 0; j < buf->len / 4; ++j)
		mmt_log_cont(" 0x%08x", ((uint32_t *)buf->data)[j]);
	for (j = buf->len / 4 * 4; j < buf->len; ++j)
		mmt_log_cont(" 0x%02x", buf->data[j]);
	mmt_log_cont_nl();
}

void dump_mmt_buf_as_word_pairs(struct mmt_buf *buf, const char *(*fun)(uint32_t))
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

void dump_mmt_buf_as_words_desc(struct mmt_buf *buf, const char *(*fun)(uint32_t))
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

void dump_args(struct mmt_memory_dump *args, int argc, uint64_t except_ptr)
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
