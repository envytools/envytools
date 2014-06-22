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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mmt_bin2text.h"

unsigned char buf[BUF_SIZE];
int idx = 0;
static int len = 0;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define EOR 10

struct mmt_write
{
	struct mmt_message msg_type;
	uint32_t id;
	uint32_t offset;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct mmt_read
{
	struct mmt_message msg_type;
	uint32_t id;
	uint32_t offset;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct mmt_mmap
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t start;
	uint64_t len;
} __packed;

struct mmt_unmap
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t start;
	uint64_t len;
	uint64_t data1;
	uint64_t data2;
} __packed;

struct mmt_open
{
	struct mmt_message msg_type;
	uint32_t flags;
	uint32_t mode;
	uint32_t ret;
	struct mmt_buf path;
} __packed;

struct mmt_mremap
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t old_start;
	uint64_t old_len;
	uint64_t data1;
	uint64_t data2;
	uint64_t start;
	uint64_t len;
} __packed;

void *load_data(int sz)
{
	if (idx + sz < len)
		return buf + idx;

	if (idx > 0)
	{
		len -= idx;
		memmove(buf, buf + idx, len);
		idx = 0;
	}

	while (idx + sz >= len)
	{
		int r = read(0, buf + len, BUF_SIZE - len);
		if (r < 0)
		{
			perror("read");
			exit(1);
		}
		else if (r == 0)
		{
			fprintf(stderr, "EOF\n");
			fflush(stderr);
			exit(0);
		}
		len += r;
	}

	return buf + idx;
}

void dump_next()
{
	int i, limit = MIN(idx + 50, len);
	for (i = idx; i < limit; ++i)
		fprintf(stderr, "%02x ", buf[i]);
	fprintf(stderr, "\n");
	for (i = idx; i < limit; ++i)
		fprintf(stderr, " %c ", isprint(buf[i]) ? buf[i] : '?');
	fprintf(stderr, "\n");
}

void check_eor(uint8_t e)
{
	if (e != EOR)
	{
		fprintf(stderr, "message does not end with EOR byte: 0x%02x\n", e);
		dump_next();
		abort();
	}
}

int main()
{
	while (1)
	{
		struct mmt_message *msg = load_data(1);

		if (msg->type == '=')
		{
			while (buf[idx] != 10)
			{
				idx++;
				load_data(1);
			}
			idx++;
		}
		else if (msg->type == 'r') // read
		{
			struct mmt_read *w;
			w = load_data(sizeof(struct mmt_read) + 1);
			w = load_data(sizeof(struct mmt_read) + 1 + w->len);

			check_eor(buf[idx + sizeof(*w) + w->len]);

			if (PRINT_DATA)
			{
				if (w->len == 1)
					fprintf(stdout, PFX "r %d:0x%04x, 0x%02x \n", w->id, w->offset, w->data[0]);
				else if (w->len == 2)
					fprintf(stdout, PFX "r %d:0x%04x, 0x%04x \n", w->id, w->offset, *(uint16_t *)(w->data));
				else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
				{
					int i;
					// text output of mmt spits words in different order depending on bitness!
					// so to reduce possible confusion on dedma end, do NOT use multi word format
					for (i = 0; i < w->len; i += 4)
						fprintf(stdout, PFX "r %d:0x%04x, 0x%08x \n", w->id, w->offset + i, *(uint32_t *)(w->data + i));
				}
				else
				{
					fprintf(stderr, "unk size: %d\n", w->len);
					abort();
				}
			}
			else
			{
				if (w->len != 1 && w->len != 2 && w->len != 4 && w->len != 8 && w->len != 16 && w->len != 32)
				{
					fprintf(stderr, "unk size: %d\n", w->len);
					abort();
				}
			}

			idx += sizeof(*w) + w->len + 1;
		}
		else if (msg->type == 'w') // write
		{
			struct mmt_write *w;
			w = load_data(sizeof(struct mmt_write) + 1);
			w = load_data(sizeof(struct mmt_write) + 1 + w->len);

			check_eor(buf[idx + sizeof(*w) + w->len]);

			if (PRINT_DATA)
			{
				if (w->len == 1)
					fprintf(stdout, PFX "w %d:0x%04x, 0x%02x \n", w->id, w->offset, w->data[0]);
				else if (w->len == 2)
					fprintf(stdout, PFX "w %d:0x%04x, 0x%04x \n", w->id, w->offset, *(uint16_t *)(w->data));
				else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
				{
					int i;
					for (i = 0; i < w->len; i += 4)
						fprintf(stdout, PFX "w %d:0x%04x, 0x%08x \n", w->id, w->offset + i, *(uint32_t *)(w->data + i));
				}
				else
				{
					fprintf(stderr, "unk size: %d\n", w->len);
					abort();
				}
			}
			else
			{
				if (w->len != 1 && w->len != 2 && w->len != 4 && w->len != 8 && w->len != 16 && w->len != 32)
				{
					fprintf(stderr, "unk size: %d\n", w->len);
					abort();
				}
			}

			idx += sizeof(*w) + w->len + 1;
		}
		else if (msg->type == 'm') // mmap
		{
			struct mmt_mmap *mm = load_data(sizeof(struct mmt_mmap) + 1);

			check_eor(buf[idx + sizeof(*mm)]);

			if (PRINT_DATA)
				fprintf(stdout, PFX "got new mmap at %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n", (void *)mm->start, mm->len,
						(long long unsigned int) mm->offset, mm->id);

			idx += sizeof(*mm) + 1;
		}
		else if (msg->type == 'u') // unmap
		{
			struct mmt_unmap *mm = load_data(sizeof(struct mmt_unmap) + 1);

			check_eor(buf[idx + sizeof(*mm)]);

			if (PRINT_DATA)
				fprintf(stdout, PFX "removed mmap 0x%lx:0x%lx for: %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
						mm->data1, mm->data2, (void *)mm->start, mm->len, (long long unsigned int) mm->offset, mm->id);

			idx += sizeof(*mm) + 1;
		}
		else if (msg->type == 'e') // mremap
		{
			struct mmt_mremap *mm = load_data(sizeof(struct mmt_mremap) + 1);

			check_eor(buf[idx + sizeof(*mm)]);

			if (PRINT_DATA)
				fprintf(stdout, PFX "changed mmap 0x%lx:0x%lx from: (address: %p, len: 0x%08lx), to: (address: %p, len: 0x%08lx), offset 0x%llx, serial %d\n",
						mm->data1, mm->data2,
						(void *)mm->old_start, mm->old_len,
						(void *)mm->start, mm->len,
						(long long unsigned int) mm->offset, mm->id);

			idx += sizeof(*mm) + 1;
		}
		else if (msg->type == 'o') // open
		{
			struct mmt_open *o;
			o = load_data(sizeof(struct mmt_open) + 1);
			o = load_data(sizeof(struct mmt_open) + 1 + o->path.len);

			check_eor(buf[idx + sizeof(*o) + o->path.len]);

			if (PRINT_DATA)
				fprintf(stdout, PFX "sys_open: %s, flags: 0x%x, mode: 0x%x, ret: %d\n", o->path.data, o->flags, o->mode, o->ret);

			idx += sizeof(*o) + o->path.len + 1;
		}
		else if (msg->type == 'n') // nvidia / nouveau
			parse_nvidia();
		else
		{
			fprintf(stderr, "unknown type: 0x%x\n", msg->type);
			fprintf(stderr, "%c\n", msg->type);
			dump_next();
			abort();
		}
	}

	return 0;
}
