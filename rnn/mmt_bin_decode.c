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

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

unsigned char mmt_buf[MMT_BUF_SIZE];
int mmt_idx = 0;
static int len = 0;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define EOR 10

void *mmt_load_data(int sz)
{
	if (mmt_idx + sz < len)
		return mmt_buf + mmt_idx;

	if (mmt_idx > 0)
	{
		len -= mmt_idx;
		memmove(mmt_buf, mmt_buf + mmt_idx, len);
		mmt_idx = 0;
	}

	while (mmt_idx + sz >= len)
	{
		int r = read(0, mmt_buf + len, MMT_BUF_SIZE - len);
		if (r < 0)
		{
			perror("read");
			exit(1);
		}
		else if (r == 0)
		{
			fprintf(stderr, "EOF\n");
			fflush(stderr);
			return NULL;
		}
		len += r;
	}

	return mmt_buf + mmt_idx;
}

void mmt_dump_next()
{
	int i, limit = MIN(mmt_idx + 50, len);
	for (i = mmt_idx; i < limit; ++i)
		fprintf(stderr, "%02x ", mmt_buf[i]);
	fprintf(stderr, "\n");
	for (i = mmt_idx; i < limit; ++i)
		fprintf(stderr, " %c ", isprint(mmt_buf[i]) ? mmt_buf[i] : '?');
	fprintf(stderr, "\n");
}

void mmt_check_eor(int size)
{
	uint8_t e = mmt_buf[mmt_idx + size - 1];
	if (e != EOR)
	{
		fprintf(stderr, "message does not end with EOR byte: 0x%02x\n", e);
		mmt_dump_next();
		abort();
	}
}

void mmt_decode(const struct mmt_decode_funcs *funcs, void *state)
{
	int size;
	while (1)
	{
		struct mmt_message *msg = mmt_load_data(1);
		if (msg == NULL)
			return;

		if (msg->type == '=')
		{
			while (mmt_buf[mmt_idx] != 10)
			{
				mmt_idx++;
				if (mmt_load_data(1) == NULL)
					return;
			}
			mmt_idx++;
		}
		else if (msg->type == 'r') // read
		{
			struct mmt_read *w;
			size = sizeof(struct mmt_read) + 1;
			w = mmt_load_data(size);
			size += w->len;
			w = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->memread)
				funcs->memread(w, state);

			mmt_idx += size;
		}
		else if (msg->type == 'w') // write
		{
			struct mmt_write *w;
			size = sizeof(struct mmt_write) + 1;
			w = mmt_load_data(size);
			size += w->len;
			w = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->memwrite)
				funcs->memwrite(w, state);

			mmt_idx += size;
		}
		else if (msg->type == 'm') // mmap
		{
			size = sizeof(struct mmt_mmap) + 1;
			struct mmt_mmap *mm = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->mmap)
				funcs->mmap(mm, state);

			mmt_idx += size;
		}
		else if (msg->type == 'u') // unmap
		{
			size = sizeof(struct mmt_unmap) + 1;
			struct mmt_unmap *mm = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->munmap)
				funcs->munmap(mm, state);

			mmt_idx += size;
		}
		else if (msg->type == 'e') // mremap
		{
			size = sizeof(struct mmt_mremap) + 1;
			struct mmt_mremap *mm = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->mremap)
				funcs->mremap(mm, state);

			mmt_idx += size;
		}
		else if (msg->type == 'o') // open
		{
			size = sizeof(struct mmt_open) + 1;
			struct mmt_open *o;
			o = mmt_load_data(size);
			size += o->path.len;
			o = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->open)
				funcs->open(o, state);

			mmt_idx += size;
		}
		else if (msg->type == 'n') // nvidia / nouveau
			mmt_decode_nvidia((struct mmt_nvidia_decode_funcs *)funcs, state);
		else
		{
			fprintf(stderr, "unknown type: 0x%x\n", msg->type);
			fprintf(stderr, "%c\n", msg->type);
			mmt_dump_next();
			abort();
		}
	}
}

