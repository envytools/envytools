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

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "buffer.h"
#include "config.h"
#include "demmt.h"
#include "drm.h"
#include "macro.h"
#include "nvrm.h"
#include "object_state.h"
#include "util.h"
#include "log.h"

struct rnndomain *domain;
struct rnndb *rnndb;
struct rnndb *rnndb_g80_texture;
static struct rnndb *rnndb_gf100_shaders;
struct rnndb *rnndb_nvrm_object;
struct rnndeccontext *gf100_shaders_ctx;
struct rnndomain *tsc_domain;
struct rnndomain *tic_domain;
struct rnndomain *gf100_vp_header_domain, *gf100_fp_header_domain,
	*gf100_gp_header_domain, *gf100_tcp_header_domain, *gf100_tep_header_domain;

const struct envy_colors *colors = NULL;
int mmt_sync_fd = -1;

static void demmt_memread(struct mmt_read *w, void *state)
{
	char comment[50];

	buffer_register_mmt_read(w);

	struct cpu_mapping *mapping = cpu_mappings[w->id];
	uint64_t gpu_addr = cpu_mapping_to_gpu_addr(mapping, w->offset);
	if (print_gpu_addresses && gpu_addr)
		sprintf(comment, " (gpu=0x%08" PRIx64 ")", gpu_addr);
	else
		comment[0] = 0;

	if (dump_memory_reads)
	{
		unsigned char *data = &w->data[0];
		if (w->len == 1)
			mmt_printf("r %d:0x%04x%s, 0x%02x\n", w->id, w->offset, comment, data[0]);
		else if (w->len == 2)
			mmt_printf("r %d:0x%04x%s, 0x%04x\n", w->id, w->offset, comment, *(uint16_t *)&data[0]);
		else if (w->len == 4 || w->len == 8 || w->len == 16 || w->len == 32)
		{
			mmt_printf("r %d:0x%04x%s, ", w->id, w->offset, comment);
			int i;
			for (i = 0; i < w->len; i += 4)
				mmt_printf("0x%08x ", *(uint32_t *)&data[i]);
			mmt_printf("%s\n", "");
		}
		else
		{
			mmt_error("unhandled size: %d\n", w->len);
			abort();
		}
	}
}

static void demmt_memwrite(struct mmt_write *w, void *state)
{
	buffer_register_mmt_write(w);
}

static void *mem2_buffer;
void demmt_memread2(struct mmt_read2 *r2, void *state)
{
	int i;
	struct cpu_mapping *m = NULL;
	for (i = 0; i < max_id + 1; ++i)
	{
		m = cpu_mappings[i];
		if (m && r2->addr >= m->cpu_addr && r2->addr < m->cpu_addr + m->length)
			break;
	}

	if (i != max_id + 1)
	{
		if (!mem2_buffer)
			mem2_buffer = malloc(4096);
		struct mmt_read *r1 = mem2_buffer;
		r1->msg_type = r2->msg_type;
		r1->id = i;
		r1->offset = r2->addr - m->cpu_addr;
		r1->len = r2->len;
		memcpy(r1->data, r2->data, r1->len);
		demmt_memread(r1, state);
	}

	if (dump_memory_reads)
	{
		unsigned char *data = &r2->data[0];
		if (r2->len == 1)
			mmt_printf("@r 0x%" PRIx64 ", 0x%02x\n", r2->addr, data[0]);
		else if (r2->len == 2)
			mmt_printf("@r 0x%" PRIx64 ", 0x%04x\n", r2->addr, *(uint16_t *)&data[0]);
		else if (r2->len == 4 || r2->len == 8 || r2->len == 16 || r2->len == 32)
		{
			mmt_printf("@r 0x%" PRIx64 ", ", r2->addr);
			int i;
			for (i = 0; i < r2->len; i += 4)
				mmt_printf("0x%08x ", *(uint32_t *)&data[i]);
			mmt_printf("%s\n", "");
		}
		else
		{
			mmt_error("unhandled size: %d\n", r2->len);
			abort();
		}
	}
}

void demmt_memwrite2(struct mmt_write2 *w2, void *state)
{
	int i;
	struct cpu_mapping *m = NULL;
	for (i = 0; i < max_id + 1; ++i)
	{
		m = cpu_mappings[i];
		if (m && w2->addr >= m->cpu_addr && w2->addr < m->cpu_addr + m->length)
			break;
	}

	if (i != max_id + 1)
	{
		if (!mem2_buffer)
			mem2_buffer = malloc(4096);
		struct mmt_write *w1 = mem2_buffer;
		w1->msg_type = w2->msg_type;
		w1->id = i;
		w1->offset = w2->addr - m->cpu_addr;
		w1->len = w2->len;
		memcpy(w1->data, w2->data, w1->len);
		demmt_memwrite(w1, state);
	}

	if (dump_memory_writes)
	{
		unsigned char *data = &w2->data[0];
		if (w2->len == 1)
			mmt_printf("@w 0x%" PRIx64 ", 0x%02x\n", w2->addr, data[0]);
		else if (w2->len == 2)
			mmt_printf("@w 0x%" PRIx64 ", 0x%04x\n", w2->addr, *(uint16_t *)&data[0]);
		else if (w2->len == 4 || w2->len == 8 || w2->len == 16 || w2->len == 32)
		{
			mmt_printf("@w 0x%" PRIx64 ", ", w2->addr);
			int i;
			for (i = 0; i < w2->len; i += 4)
				mmt_printf("0x%08x ", *(uint32_t *)&data[i]);
			mmt_printf("%s\n", "");
		}
		else
		{
			mmt_error("unhandled size: %d\n", w2->len);
			abort();
		}
	}
}

static void demmt_mmap(struct mmt_mmap *mm, void *state)
{
	__demmt_mmap(mm->start, mm->len, mm->id, mm->offset, state);
}

static struct bitfield_desc mmap_prot[] =
{
		{PROT_READ, "READ"},
		{PROT_WRITE, "WRITE"},
		{PROT_EXEC, "EXEC"},
		{0, NULL}
};

void decode_bitfield(uint32_t data, struct bitfield_desc *bfdesc)
{
	uint32_t data_ = data;
	int printed = 0;

	if (!data)
		mmt_log_cont("%s", "NONE");

	while (data && bfdesc->name)
	{
		if (data & bfdesc->mask)
		{
			mmt_log_cont("%s%s", printed ? ", " : "", bfdesc->name);
			data &= ~bfdesc->mask;
			printed = 1;
		}
		bfdesc++;
	}

	if (data)
		mmt_log_cont("%sUNK%x", printed ? ", " : "", data);

	mmt_log_cont(" (0x%x)", data_);
}

void decode_mmap_prot(uint32_t prot)
{
	decode_bitfield(prot, mmap_prot);
}

static struct bitfield_desc mmap_flags[] =
{
		{MAP_SHARED, "SHARED"},
		{MAP_PRIVATE, "PRIVATE"},
		{MAP_FIXED, "FIXED"},
		//...
		{0, NULL}
};

void decode_mmap_flags(uint32_t flags)
{
	decode_bitfield(flags, mmap_flags);
}

static void demmt_mmap2(struct mmt_mmap2 *mm, void *state)
{
	__demmt_mmap2(mm->start, mm->len, mm->id, mm->offset, mm->fd, mm->prot, mm->flags, state);
}

static void demmt_munmap(struct mmt_unmap *mm, void *state)
{
	buffer_flush();

	if (dump_sys_munmap)
		mmt_log("munmap: address: 0x%" PRIx64 ", length: 0x%08" PRIx64 ", id: %d, offset: 0x%08" PRIx64 "",
				mm->start, mm->len, mm->id, mm->offset);

	nvrm_munmap(mm->id, mm->start, mm->len, mm->offset);
}

static void demmt_mremap(struct mmt_mremap *mm, void *state)
{
	buffer_flush();

	if (dump_sys_mremap)
		mmt_log("mremap: old_address: 0x%" PRIx64 ", new_address: 0x%" PRIx64 ", old_length: 0x%08" PRIx64 ", new_length: 0x%08" PRIx64 ", id: %d, offset: 0x%08" PRIx64 "\n",
				mm->old_start, mm->start, mm->old_len, mm->len, mm->id, mm->offset);

	buffer_mremap(mm);
}

static void demmt_open(struct mmt_open *o, void *state)
{
	buffer_flush();

	if (dump_sys_open)
		mmt_log("sys_open: %s, flags: 0x%x, mode: 0x%x, ret: %d\n", o->path.data, o->flags, o->mode, o->ret);
}

static void demmt_msg(uint8_t *data, int len, void *state)
{
	buffer_flush();

	if (dump_msg)
	{
		mmt_log("MSG: %s", "");
		fwrite(data, 1, len, stdout);
		mmt_log_cont_nl();
	}
}

static void demmt_write_syscall(struct mmt_write_syscall *o, void *state)
{
	buffer_flush();

	if (dump_sys_write)
		fwrite(o->data.data, 1, o->data.len, stdout);
}

static void demmt_dup_syscall(struct mmt_dup_syscall *o, void *state)
{
	buffer_flush();

	if (dump_sys_open)
		mmt_log("sys_dup: old: %d, new: %d\n", o->oldfd, o->newfd);
}

static void demmt_sync(struct mmt_sync *o, void *state)
{
	if (mmt_sync_fd == -1)
		return;

	fflush(stdout);
	fdatasync(1);
	int cnt = 4;
	while (cnt)
	{
		int r = write(mmt_sync_fd, ((char *)&o->id) + 4 - cnt, cnt);
		if (r > 0)
			cnt -= r;
		else if (r != EINTR)
			abort();
	}
	fdatasync(mmt_sync_fd);
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

static void __demmt_ioctl_pre(uint32_t fd, uint32_t id, struct mmt_buf *data, void *state, struct mmt_memory_dump *args, int argc)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(id, &dir, &type, &nr, &size);
	int print_raw = 1;

	if (type == 0x64) // DRM
	{
		is_nouveau = 1;
		print_raw = demmt_drm_ioctl_pre(fd, dir, nr, size, data, state, args, argc);
	}
	else if (type == 0x46) // nvidia
		print_raw = nvrm_ioctl_pre(fd, id, dir, nr, size, data, state, args, argc);

	print_raw = print_raw || dump_raw_ioctl_data;

	if (print_raw)
	{
		mmt_log("ioctl pre  0x%02x (0x%08x), fd: %d, dir: %2s, size: %4d",
				nr, id, fd, dir_desc[dir], size);
		if (size != data->len)
			mmt_log_cont(", data.len: %d", data->len);

		ioctl_data_print(data);
	}

	buffer_ioctl_pre(print_raw);
}

static void __demmt_ioctl_post(uint32_t fd, uint32_t id, struct mmt_buf *data,
		uint64_t ret, uint64_t err, void *state, struct mmt_memory_dump *args, int argc)
{
	uint8_t dir, type, nr;
	uint16_t size;
	decode_ioctl_id(id, &dir, &type, &nr, &size);
	int print_raw = 0;

	if (type == 0x64) // DRM
		print_raw = demmt_drm_ioctl_post(fd, dir, nr, size, data, ret, err, state, args, argc);
	else if (type == 0x46) // nvidia
		print_raw = nvrm_ioctl_post(fd, id, dir, nr, size, data, ret, err, state, args, argc);

	print_raw = print_raw || dump_raw_ioctl_data;

	if (print_raw)
	{
		mmt_log("ioctl post 0x%02x (0x%08x), fd: %d, dir: %2s, size: %4d",
				nr, id, fd, dir_desc[dir], size);
		if (ret)
			mmt_log_cont(", ret: 0x%" PRIx64 "", ret);
		if (err)
			mmt_log_cont(", err: 0x%" PRIx64 "", err);
		if (size != data->len)
			mmt_log_cont(", data.len: %d", data->len);
		ioctl_data_print(data);
		mmt_log_cont("%s\n", "");
	}
}

void demmt_ioctl_pre(struct mmt_ioctl_pre *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__demmt_ioctl_pre(ctl->fd, ctl->id, &ctl->data, state, args, argc);
}

void demmt_ioctl_pre_v2(struct mmt_ioctl_pre_v2 *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__demmt_ioctl_pre(ctl->fd, ctl->id, &ctl->data, state, args, argc);
}

void demmt_ioctl_post(struct mmt_ioctl_post *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__demmt_ioctl_post(ctl->fd, ctl->id, &ctl->data, 0, 0, state, args, argc);
}

void demmt_ioctl_post_v2(struct mmt_ioctl_post_v2 *ctl, void *state, struct mmt_memory_dump *args, int argc)
{
	__demmt_ioctl_post(ctl->fd, ctl->id, &ctl->data, ctl->ret, ctl->err, state, args, argc);
}

const struct mmt_nvidia_decode_funcs demmt_funcs =
{
	{ demmt_memread, demmt_memwrite, demmt_mmap, demmt_mmap2, demmt_munmap,
	  demmt_mremap, demmt_open, demmt_msg, demmt_write_syscall, demmt_dup_syscall,
	  demmt_sync, demmt_ioctl_pre_v2, demmt_ioctl_post_v2, demmt_memread2,
	  demmt_memwrite2 },
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

uint64_t roundup_to_pagesize(uint64_t sz)
{
	static uint64_t pg = 0;
	if (!pg)
		pg = sysconf(_SC_PAGESIZE);
	return (sz + pg - 1) & ~(pg - 1);
}

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

	if (pager_enabled)
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

	mmt_decode(&demmt_funcs.base, NULL);
	buffer_flush();
	fflush(stdout);

	fini_macrodis();
	demmt_cleanup_isas();
	rnndec_freecontext(gf100_shaders_ctx);
	rnn_freedb(rnndb);
	rnn_freedb(rnndb_g80_texture);
	rnn_freedb(rnndb_gf100_shaders);
	rnn_freedb(rnndb_nvrm_object);
	rnn_fini();

	return 0;
}
