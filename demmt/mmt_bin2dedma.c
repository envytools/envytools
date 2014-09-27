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

#include <stdio.h>
#include <stdlib.h>

#include "mmt_bin2dedma.h"
#include "mmt_bin_decode.h"

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

void txt_mmap(struct mmt_mmap *mm, void *state)
{
	fprintf(stdout, PFX "got new mmap at %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
			(void *)mm->start, mm->len, (long long unsigned int) mm->offset,
			mm->id);
}

void txt_mmap2(struct mmt_mmap2 *mm, void *state)
{
	fprintf(stdout, PFX "got new mmap at %p, len: 0x%08lx, offset: 0x%llx, serial: %d, fd: %d\n",
			(void *)mm->start, mm->len, (long long unsigned int) mm->offset,
			mm->id, mm->fd);
}

void txt_munmap(struct mmt_unmap *mm, void *state)
{
	fprintf(stdout, PFX "removed mmap 0x%lx:0x%lx for: %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
				mm->data1, mm->data2, (void *)mm->start, mm->len,
				(long long unsigned int) mm->offset, mm->id);
}

void txt_mremap(struct mmt_mremap *mm, void *state)
{
	fprintf(stdout, PFX "changed mmap 0x%lx:0x%lx from: (address: %p, len: 0x%08lx), to: (address: %p, len: 0x%08lx), offset 0x%llx, serial %d\n",
				mm->data1, mm->data2,
				(void *)mm->old_start, mm->old_len,
				(void *)mm->start, mm->len,
				(long long unsigned int) mm->offset, mm->id);
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

#define PRINT_DATA 1

int main()
{
	if (PRINT_DATA)
		mmt_decode(&txt_nvidia_funcs.base, &mmt_txt_nv_state);
	else
		mmt_decode(&txt_nvidia_funcs_empty.base, NULL);
	return 0;
}
