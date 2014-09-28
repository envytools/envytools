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
#include <unistd.h>

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "buffer.h"
#include "config.h"
#include "demmt.h"
#include "drm.h"
#include "nvrm.h"
#include "util.h"
#include "log.h"

struct rnndomain *domain;
struct rnndb *rnndb;
static struct rnndb *rnndb_g80_texture;
static struct rnndb *rnndb_gf100_shaders;
struct rnndb *rnndb_nvrm_object;
struct rnndeccontext *g80_texture_ctx;
struct rnndeccontext *gf100_shaders_ctx;
struct rnndomain *tsc_domain;
struct rnndomain *tic_domain;
struct rnndomain *gf100_vp_header_domain, *gf100_fp_header_domain,
	*gf100_gp_header_domain, *gf100_tcp_header_domain, *gf100_tep_header_domain;

const struct envy_colors *colors = NULL;

static void demmt_memread(struct mmt_read *w, void *state)
{
	buffer_register_mmt_read(w);

	char comment[50];
	struct buffer *buf = buffers[w->id];
	if (print_gpu_addresses && buf->gpu_start)
		sprintf(comment, " (gpu=0x%08lx)", buf->gpu_start + w->offset);
	else
		comment[0] = 0;

	if (dump_memory_reads)
	{
		unsigned char *data = &w->data[0];
		if (w->len == 1)
			fprintf(stdout, "r %d:0x%04x%s, 0x%02x\n", w->id, w->offset, comment, data[0]);
		else if (w->len == 2)
			fprintf(stdout, "r %d:0x%04x%s, 0x%04x\n", w->id, w->offset, comment, *(uint16_t *)&data[0]);
		else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
		{
			fprintf(stdout, "r %d:0x%04x%s, ", w->id, w->offset, comment);
			int i;
			for (i = 0; i < w->len; i += 4)
				fprintf(stdout, "0x%08x ", *(uint32_t *)&data[i]);
			fprintf(stdout, "\n");
		}
	}
}

static void demmt_memwrite(struct mmt_write *w, void *state)
{
	buffer_register_mmt_write(w);
}

static void demmt_mmap(struct mmt_mmap *mm, void *state)
{
	if (dump_sys_mmap)
		mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx\n",
				(void *)mm->start, mm->len, mm->id, mm->offset);

	buffer_mmap(mm->id, mm->start, mm->len, mm->offset, NULL, NULL);
}

static void demmt_mmap2(struct mmt_mmap2 *mm, void *state)
{
	if (dump_sys_mmap)
		mmt_log("mmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, fd: %d\n",
				(void *)mm->start, mm->len, mm->id, mm->offset, mm->fd);

	buffer_mmap(mm->id, mm->start, mm->len, mm->offset, NULL, NULL);
}

static void demmt_munmap(struct mmt_unmap *mm, void *state)
{
	if (dump_sys_munmap)
		mmt_log("munmap: address: %p, length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
				(void *)mm->start, mm->len, mm->id, mm->offset, mm->data1, mm->data2);

	buffer_munmap(mm->id);
}

static void demmt_mremap(struct mmt_mremap *mm, void *state)
{
	if (dump_sys_mremap)
		mmt_log("mremap: old_address: %p, new_address: %p, old_length: 0x%08lx, new_length: 0x%08lx, id: %d, offset: 0x%08lx, data1: 0x%08lx, data2: 0x%08lx\n",
				(void *)mm->old_start, (void *)mm->start, mm->old_len, mm->len, mm->id, mm->offset, mm->data1, mm->data2);

	buffer_mremap(mm);
}

static void demmt_open(struct mmt_open *o, void *state)
{
	if (dump_sys_open)
		mmt_log("sys_open: %s, flags: 0x%x, mode: 0x%x, ret: %d\n", o->path.data, o->flags, o->mode, o->ret);
}

static void demmt_msg(uint8_t *data, int len, void *state)
{
	if (dump_msg)
	{
		mmt_log("MSG: %s", "");
		fwrite(data, 1, len, stdout);
		fprintf(stdout, "\n");
	}
}

static void demmt_write_syscall(struct mmt_write_syscall *o, void *state)
{
	if (dump_sys_write)
		fwrite(o->data.data, 1, o->data.len, stdout);
}

static void ioctl_data_print(struct mmt_buf *data)
{
	uint32_t i;
	if (!dump_raw_ioctl_data)
		return;

	mmt_log_cont(", data:%s", "");
	for (i = 0; i < data->len / 4; ++i)
		mmt_log_cont(" 0x%08x", ((uint32_t *)data->data)[i]);
}

static void decode_ioctl_id(uint32_t id, uint8_t *dir, uint8_t *type, uint8_t *nr, uint16_t *size)
{
	*nr =    id & 0x000000ff;
	*type = (id & 0x0000ff00) >> 8;
	*size = (id & 0x3fff0000) >> 16;
	*dir =  (id & 0xc0000000) >> 30;
}

static char *dir_desc[] = { "?", "w", "r", "rw" };

void demmt_ioctl_pre(struct mmt_ioctl_pre *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(ctl->id, &dir, &type, &nr, &size);
	int print_raw = 1;

	if (type == 0x64) // DRM
	{
		is_nouveau = 1;
		print_raw = demmt_drm_ioctl_pre(dir, nr, size, &ctl->data, state);
	}
	else if (type == 0x46) // nvidia
		print_raw = demmt_nv_ioctl_pre(ctl->fd, ctl->id, dir, nr, size, &ctl->data, state, args, argc);

	print_raw = print_raw || dump_raw_ioctl_data;

	if (print_raw)
	{
		mmt_log("ioctl pre  0x%02x (0x%08x), fd: %d, dir: %2s, size: %4d",
				nr, ctl->id, ctl->fd, dir_desc[dir], size);
		if (size != ctl->data.len)
			mmt_log_cont(", data.len: %d", ctl->data.len);

		ioctl_data_print(&ctl->data);
	}

	buffer_ioctl_pre(print_raw);
}

void demmt_ioctl_post(struct mmt_ioctl_post *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(ctl->id, &dir, &type, &nr, &size);
	int print_raw = 0;

	if (type == 0x64) // DRM
		print_raw = demmt_drm_ioctl_post(dir, nr, size, &ctl->data, state);
	else if (type == 0x46) // nvidia
		print_raw = demmt_nv_ioctl_post(ctl->fd, ctl->id, dir, nr, size, &ctl->data, state, args, argc);

	print_raw = print_raw || dump_raw_ioctl_data;

	if (print_raw)
	{
		mmt_log("ioctl post 0x%02x (0x%08x), fd: %d, dir: %2s, size: %4d",
				nr, ctl->id, ctl->fd, dir_desc[dir], size);
		if (size != ctl->data.len)
			mmt_log_cont(", data.len: %d", ctl->data.len);
		ioctl_data_print(&ctl->data);
		mmt_log_cont("%s\n", "");
	}
}

const struct mmt_nvidia_decode_funcs demmt_funcs =
{
	{ demmt_memread, demmt_memwrite, demmt_mmap, demmt_mmap2, demmt_munmap, demmt_mremap, demmt_open, demmt_msg, demmt_write_syscall },
	NULL,
	NULL,
	demmt_ioctl_pre,
	demmt_ioctl_post,
	demmt_memory_dump,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	demmt_nv_mmap,
	demmt_nv_mmap2,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	demmt_nv_call_method_data,
	demmt_nv_ioctl_4d,
	demmt_nv_mmiotrace_mark,
	demmt_nouveau_gem_pushbuf_data
};

int main(int argc, char *argv[])
{
	char *filename = read_opts(argc, argv);

	/* set up an rnn context */
	rnn_init();
	rnndb = rnn_newdb();
	rnn_parsefile(rnndb, "fifo/nv_objects.xml");
	if (rnndb->estatus)
		abort();
	rnn_prepdb(rnndb);
	domain = rnn_finddomain(rnndb, "SUBCHAN");
	if (!domain)
		abort();

	rnndb_g80_texture = rnn_newdb();
	rnn_parsefile(rnndb_g80_texture, "graph/g80_texture.xml");
	if (rnndb_g80_texture->estatus)
		abort();
	rnn_prepdb(rnndb_g80_texture);

	g80_texture_ctx = rnndec_newcontext(rnndb_g80_texture);
	g80_texture_ctx->colors = colors;

	rnndb_gf100_shaders = rnn_newdb();
	rnn_parsefile(rnndb_gf100_shaders, "graph/gf100_shaders.xml");
	if (rnndb_gf100_shaders->estatus)
		abort();
	rnn_prepdb(rnndb_gf100_shaders);

	gf100_shaders_ctx = rnndec_newcontext(rnndb_gf100_shaders);
	gf100_shaders_ctx->colors = colors;

	rnndb_nvrm_object = rnn_newdb();
	rnn_parsefile(rnndb_nvrm_object, "../docs/nvrm/rnndb/nvrm_object.xml");
	if (rnndb_nvrm_object->estatus)
		abort();
	rnn_prepdb(rnndb_nvrm_object);

	struct rnnvalue *v = NULL;
	struct rnnenum *chs = rnn_findenum(rnndb, "chipset");
	FINDARRAY(chs->vals, v, v->value == (uint64_t)chipset);
	rnndec_varadd(g80_texture_ctx, "chipset", v ? v->name : "NV1");
	tic_domain = rnn_finddomain(rnndb_g80_texture, "TIC");
	tsc_domain = rnn_finddomain(rnndb_g80_texture, "TSC");

	gf100_vp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_VP_HEADER");
	gf100_fp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_FP_HEADER");
	gf100_gp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_GP_HEADER");
	gf100_tcp_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_TCP_HEADER");
	gf100_tep_header_domain = rnn_finddomain(rnndb_gf100_shaders, "GF100_TEP_HEADER");
	if (!gf100_vp_header_domain || !gf100_fp_header_domain || !gf100_gp_header_domain ||
			!gf100_tcp_header_domain || !gf100_tep_header_domain)
		abort();

	if (filename)
	{
		close(0);
		if (open_input(filename) == NULL)
		{
			perror("open");
			exit(1);
		}
		free(filename);
	}

	if (isatty(1))
	{
		int pipe_fds[2];
		pid_t pid;

		if (pipe(pipe_fds) < 0)
		{
			perror("pipe");
			abort();
		}

		pid = fork();
		if (pid < 0)
		{
			perror("fork");
			abort();
		}

		if (pid > 0)
		{
			char *less_argv[] = { "less", "-ScR", NULL };

			close(pipe_fds[1]);
			dup2(pipe_fds[0], 0);
			close(pipe_fds[0]);
			execvp(less_argv[0], less_argv);

			perror("exec");
			abort();
		}

		close(pipe_fds[0]);
		dup2(pipe_fds[1], 1);
		dup2(pipe_fds[1], 2);
		close(pipe_fds[1]);
	}
	fprintf(stdout, "Chipset: NV%02X\n", chipset);

	mmt_decode(&demmt_funcs.base, NULL);
	buffer_flush();
	fflush(stdout);
	return 0;
}
