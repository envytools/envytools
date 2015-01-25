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
unsigned int mmt_idx = 0;
static unsigned int len = 0;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define EOR 10

struct mmt_buf *find_ptr(uint64_t ptr, struct mmt_memory_dump *args, int argc)
{
	int i;
	if (!ptr)
		return NULL;
	for (i = 0; i < argc; ++i)
		if (args[i].addr == ptr)
			return args[i].data;

	return NULL;
}

void *mmt_load_data_with_prefix(unsigned int sz, unsigned int pfx, int eof_allowed)
{
	if (pfx + mmt_idx + sz < len)
		return mmt_buf + pfx + mmt_idx;
	if (pfx + sz > MMT_BUF_SIZE)
	{
		fflush(stdout);
		fprintf(stderr, "not enough space for message of size %d\n", pfx + sz);
		fflush(stderr);
		exit(1);
	}

	if (mmt_idx > 0)
	{
		len -= mmt_idx;
		memmove(mmt_buf, mmt_buf + mmt_idx, len);
		mmt_idx = 0;
	}

	while (pfx + mmt_idx + sz > len)
	{
		int r = read(0, mmt_buf + len, MMT_BUF_SIZE - len);
		if (r < 0)
		{
			perror("read");
			exit(1);
		}
		else if (r == 0)
		{
			fflush(stdout);
			if (eof_allowed)
				fprintf(stderr, "EOF\n");
			else
				fprintf(stderr, "unexpected EOF\n");
			fflush(stderr);

			if (!eof_allowed)
				exit(1);

			return NULL;
		}
		len += r;
	}

	return mmt_buf + pfx + mmt_idx;
}

void *mmt_load_data(unsigned int sz)
{
	return mmt_load_data_with_prefix(sz, 0, 0);
}

void *mmt_load_initial_data()
{
	return mmt_load_data_with_prefix(1, 0, 1);
}

void mmt_dump_next()
{
	unsigned int i, limit = MIN(mmt_idx + 50, len);
	for (i = mmt_idx; i < limit; ++i)
		fprintf(stderr, "%02x ", mmt_buf[i]);
	fprintf(stderr, "\n");
	for (i = mmt_idx; i < limit; ++i)
		fprintf(stderr, " %c ", isprint(mmt_buf[i]) ? mmt_buf[i] : '?');
	fprintf(stderr, "\n");
}

void mmt_check_eor(unsigned int size)
{
	uint8_t e = mmt_buf[mmt_idx + size - 1];
	if (e != EOR)
	{
		fflush(stdout);
		fprintf(stderr, "message does not end with EOR byte: 0x%02x\n", e);
		mmt_dump_next();
		exit(1);
	}
}

static unsigned int load_memory_dump_v2(unsigned int pfx, struct mmt_memory_dump_v2_prefix **dump, struct mmt_buf **buf)
{
	unsigned int size1, size2;
	struct mmt_memory_dump_v2_prefix *d;
	struct mmt_buf *b;
	*dump = NULL;
	*buf = NULL;

	struct mmt_message *msg = mmt_load_data_with_prefix(1, pfx, 1);
	if (msg == NULL || msg->type != 'y')
		return 0;

	size1 = sizeof(struct mmt_memory_dump_v2_prefix);
	d = mmt_load_data_with_prefix(size1, pfx, 0);

	size2 = 4;
	b = mmt_load_data_with_prefix(size2, size1 + pfx, 0);
	size2 += b->len + 1;
	b = mmt_load_data_with_prefix(size2, size1 + pfx, 0);

	mmt_check_eor(size2 + size1 + pfx);

	*dump = d;
	*buf = b;

	return size1 + size2;
}

void mmt_decode(const struct mmt_decode_funcs *funcs, void *state)
{
	unsigned int size;
	while (1)
	{
		struct mmt_message *msg = mmt_load_initial_data();
		if (msg == NULL)
			return;

		if (msg->type == '=' || msg->type == '-')
		{
			unsigned int len = 0;
			while (mmt_buf[mmt_idx + len] != 10)
				if (mmt_load_data(++len) == NULL)
					return;

			if (funcs->msg)
				funcs->msg(&mmt_buf[mmt_idx], len, state);

			mmt_idx += len + 1;
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
		else if (msg->type == 'R') // read2
		{
			struct mmt_read2 *w;
			size = sizeof(struct mmt_read2) + 1;
			w = mmt_load_data(size);
			size += w->len;
			w = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->memread2)
				funcs->memread2(w, state);

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
		else if (msg->type == 'W') // write2
		{
			struct mmt_write2 *w;
			size = sizeof(struct mmt_write2) + 1;
			w = mmt_load_data(size);
			size += w->len;
			w = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->memwrite2)
				funcs->memwrite2(w, state);

			mmt_idx += size;
		}
		else if (msg->type == 'M') // mmap v2
		{
			size = sizeof(struct mmt_mmap2) + 1;
			struct mmt_mmap2 *mm = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->mmap2)
				funcs->mmap2(mm, state);

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
		else if (msg->type == 't') // write syscall
		{
			struct mmt_write_syscall *w;
			size = sizeof(struct mmt_write_syscall) + 1;
			w = mmt_load_data(size);
			size += w->data.len;
			w = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->write_syscall)
				funcs->write_syscall(w, state);

			mmt_idx += size;
		}
		else if (msg->type == 'S') // sync
		{
			size = sizeof(struct mmt_sync) + 1;
			struct mmt_sync *mm = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->sync)
				funcs->sync(mm, state);

			mmt_idx += size;
		}
		else if (msg->type == 'd') // dup syscall
		{
			size = sizeof(struct mmt_dup_syscall) + 1;
			struct mmt_dup_syscall *mm = mmt_load_data(size);

			mmt_check_eor(size);

			if (funcs->dup_syscall)
				funcs->dup_syscall(mm, state);

			mmt_idx += size;
		}
		else if (msg->type == 'i') // ioctl pre
		{
#define MAX_ARGS 20
			unsigned int size2, pfx;
			struct mmt_memory_dump args[MAX_ARGS];
			struct mmt_ioctl_pre_v2 *ctl;
			int argc;

			do
			{
				size = sizeof(struct mmt_ioctl_pre_v2) + 1;
				ctl = mmt_load_data(size);
				size += ctl->data.len;
				ctl = mmt_load_data(size);

				mmt_check_eor(size);

				argc = 0;

				struct mmt_memory_dump_v2_prefix *d;
				struct mmt_buf *b;
				pfx = size;

				while ((size2 = load_memory_dump_v2(pfx, &d, &b)))
				{
					args[argc].addr = d->addr;
					args[argc].data = b;
					args[argc].str = NULL;
					argc++;
					pfx += size2;
					if (argc == MAX_ARGS)
						break;
				}
			}
			while (ctl != mmt_load_data(size));

			if (funcs->ioctl_pre)
				funcs->ioctl_pre(ctl, state, args, argc);

			mmt_idx += pfx;
		}
		else if (msg->type == 'j')
		{
			unsigned int size2, pfx;
			struct mmt_memory_dump args[MAX_ARGS];
			struct mmt_ioctl_post_v2 *ctl;
			int argc;

			do
			{
				size = sizeof(struct mmt_ioctl_post_v2) + 1;
				ctl = mmt_load_data(size);
				size += ctl->data.len;
				ctl = mmt_load_data(size);

				mmt_check_eor(size);

				argc = 0;

				struct mmt_memory_dump_v2_prefix *d;
				struct mmt_buf *b;
				pfx = size;

				while ((size2 = load_memory_dump_v2(pfx, &d, &b)))
				{
					args[argc].addr = d->addr;
					args[argc].data = b;
					args[argc].str = NULL;
					argc++;
					pfx += size2;
					if (argc == MAX_ARGS)
						break;
				}
			}
			while (ctl != mmt_load_data(size));

			if (funcs->ioctl_post)
				funcs->ioctl_post(ctl, state, args, argc);

			mmt_idx += pfx;
#undef MAX_ARGS
		}
		else
		{
			fflush(stdout);
			fprintf(stderr, "unknown type: 0x%x\n", msg->type);
			fprintf(stderr, "%c\n", msg->type);
			mmt_dump_next();
			exit(1);
		}
	}
}

