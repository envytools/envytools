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
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mmt_bin2dedma.h"
#include "mmt_bin_decode.h"

static const int interpret_args_as_text = 0;
static const int dump_addr1_as_diff = 0;

void txt_memread(struct mmt_read *w, void *state)
{
	unsigned char *data = &w->data[0];

	if (w->len == 1)
		fprintf(stdout, PFX "r %d:0x%04x, 0x%02x \n", w->id, w->offset, data[0]);
	else if (w->len == 2)
		fprintf(stdout, PFX "r %d:0x%04x, 0x%04x \n", w->id, w->offset, *(uint16_t *)&data[0]);
	else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
	{
		int i;
		// text output of mmt spits words in different order depending on bitness!
		// so to reduce possible confusion on dedma end, do NOT use multi word format
		for (i = 0; i < w->len; i += 4)
			fprintf(stdout, PFX "r %d:0x%04x, 0x%08x \n", w->id, w->offset + i, *(uint32_t *)&data[i]);
	}
	else
	{
		fflush(stdout);
		fprintf(stderr, "unk size: %d\n", w->len);
		abort();
	}
}

void txt_memread2(struct mmt_read2 *w, void *state)
{
	unsigned char *data = &w->data[0];

	if (w->len == 1)
		fprintf(stdout, PFX "r 0x%" PRIx64 ", 0x%02x\n", w->addr, data[0]);
	else if (w->len == 2)
		fprintf(stdout, PFX "r 0x%" PRIx64 ", 0x%04x\n", w->addr, *(uint16_t *)&data[0]);
	else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
	{
		int i;
		for (i = 0; i < w->len; i += 4)
			fprintf(stdout, PFX "r 0x%" PRIx64 ", 0x%08x\n", w->addr + i, *(uint32_t *)&data[i]);
	}
	else
	{
		fflush(stdout);
		fprintf(stderr, "unk size: %d\n", w->len);
		abort();
	}
}

void txt_memwrite(struct mmt_write *w, void *state)
{
	unsigned char *data = &w->data[0];

	if (w->len == 1)
		fprintf(stdout, PFX "w %d:0x%04x, 0x%02x \n", w->id, w->offset, data[0]);
	else if (w->len == 2)
		fprintf(stdout, PFX "w %d:0x%04x, 0x%04x \n", w->id, w->offset, *(uint16_t *)&data[0]);
	else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
	{
		int i;
		for (i = 0; i < w->len; i += 4)
			fprintf(stdout, PFX "w %d:0x%04x, 0x%08x \n", w->id, w->offset + i, *(uint32_t *)&data[i]);
	}
	else
	{
		fflush(stdout);
		fprintf(stderr, "unk size: %d\n", w->len);
		abort();
	}
}

void txt_memwrite2(struct mmt_write2 *w, void *state)
{
	unsigned char *data = &w->data[0];

	if (w->len == 1)
		fprintf(stdout, PFX "w 0x%" PRIx64 ", 0x%02x\n", w->addr, data[0]);
	else if (w->len == 2)
		fprintf(stdout, PFX "w 0x%" PRIx64 ", 0x%04x\n", w->addr, *(uint16_t *)&data[0]);
	else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
	{
		int i;
		for (i = 0; i < w->len; i += 4)
			fprintf(stdout, PFX "w 0x%" PRIx64 ", 0x%08x\n", w->addr + i, *(uint32_t *)&data[i]);
	}
	else
	{
		fflush(stdout);
		fprintf(stderr, "unk size: %d\n", w->len);
		abort();
	}
}

void txt_mmap(struct mmt_mmap *mm, void *state)
{
	fprintf(stdout, PFX "got new mmap at 0x%" PRIx64 ", len: 0x%08" PRIx64 ", offset: 0x%" PRIx64 ", serial: %d\n",
			mm->start, mm->len, mm->offset, mm->id);
}

void txt_mmap2(struct mmt_mmap2 *mm, void *state)
{
	fprintf(stdout, PFX "got new mmap at 0x%" PRIx64 ", len: 0x%08" PRIx64 ", offset: 0x%" PRIx64 ", serial: %d, fd: %d\n",
			mm->start, mm->len, mm->offset,	mm->id, mm->fd);
}

void txt_munmap(struct mmt_unmap *mm, void *state)
{
	fprintf(stdout, PFX "removed mmap 0x%" PRIx64 ":0x%" PRIx64 " for: 0x%" PRIx64 ", len: 0x%08" PRIx64 ", offset: 0x%" PRIx64 ", serial: %d\n",
				mm->data1, mm->data2, mm->start, mm->len, mm->offset, mm->id);
}

void txt_mremap(struct mmt_mremap *mm, void *state)
{
	fprintf(stdout, PFX "changed mmap 0x%" PRIx64 ":0x%" PRIx64 " from: (address: 0x%" PRIx64 ", len: 0x%08" PRIx64 "), to: (address: 0x%" PRIx64 ", len: 0x%08" PRIx64 "), offset 0x%" PRIx64 ", serial %d\n",
				mm->data1, mm->data2, mm->old_start, mm->old_len, mm->start,
				mm->len, mm->offset, mm->id);
}

void txt_open(struct mmt_open *o, void *state)
{
	fprintf(stdout, PFX "sys_open: %s, flags: 0x%x, mode: 0x%x, ret: %d\n", o->path.data, o->flags, o->mode, o->ret);
}

void txt_msg(uint8_t *data, int len, void *state)
{
	if (len == 0 || (data[0] != '-' && data[0] != '='))
		fprintf(stdout, PFX);
	fwrite(data, 1, len, stdout);
	fprintf(stdout, "\n");
}

void txt_write_syscall(struct mmt_write_syscall *o, void *state)
{
	fwrite(o->data.data, 1, o->data.len, stdout);
}

void txt_dup(struct mmt_dup_syscall *o, void *state)
{
	fprintf(stdout, PFX "sys_dup: old: %d, new: %d\n", o->oldfd, o->newfd);
}

void dump_args(struct mmt_memory_dump *args, int argc)
{
	int i, j;

	for (i = 0; i < argc; ++i)
	{
		struct mmt_memory_dump *arg = &args[i];
		struct mmt_buf *data = arg->data;

		if (!dump_addr1_as_diff || arg->addr != 1)
		{
			fprintf(stdout, PFX "address: 0x%llx", (unsigned long long)arg->addr);
			if (arg->str)
				fprintf(stdout, ", txt: \"%s\"", arg->str->data);
			fprintf(stdout, ", data.len: %d, data:", data->len);
			for (j = 0; j < data->len / 4; ++j)
				fprintf(stdout, " 0x%08x", ((uint32_t *)data->data)[j]);
			if (data->len & 3)
				for (j = data->len & 0xfffffffc; j < data->len; ++j)
					fprintf(stdout, " %02x", data->data[j]);

			if (interpret_args_as_text)
			{
				fprintf(stdout, " \"");
				for (j = 0; j < data->len; ++j)
					fprintf(stdout, "%c",
							(isprint(data->data[j]) || isspace(data->data[j])) ?
									data->data[j] : (data->data[j] ? '?' : ' '));

				fprintf(stdout, "\"");
			}
			fprintf(stdout, "\n");
		}

		if (dump_addr1_as_diff && arg->addr == 1)
		{
			fflush(stdout);
			static int del = 0;
			if (!del)
			{
				unlink("/tmp/mmt0");
				unlink("/tmp/mmt1");
				del = 1;
				continue;
			}
			if (rename("/tmp/mmt1", "/tmp/mmt0"))
				perror("rename");
			int fd = open("/tmp/mmt1", O_WRONLY | O_CREAT, 0660);
			if (fd >= 0)
			{
				write(fd, data->data, data->len);
				close(fd);
			}
			else
				perror("open");

			FILE *f = popen("diff /tmp/mmt0 /tmp/mmt1", "r");
			char buf[65536];

			int r, curlen = 0;
			if (f)
			{
				do
				{
					r = fread(buf + curlen, 1, 65536 - curlen, f);
					if (r > 0)
						curlen += r;
				}
				while (!feof(f));

				pclose(f);
			}
			else
				perror("popen");

			if (curlen > 0)
			{
				fprintf(stdout, "diff: \n");
				fflush(stdout);
				write(1, buf, curlen);
				if (buf[curlen - 1] != '\n')
					fprintf(stdout, "\n");
				fprintf(stdout, "end of diff\n\n");
			}
		}
	}
}

#define PRINT_DATA 1

int main()
{
	if (PRINT_DATA)
		mmt_decode(&txt_nvidia_funcs.base, &mmt_txt_nv_state);
	else
		mmt_decode(&txt_nvidia_funcs_empty.base, NULL);
	return 0;
}
