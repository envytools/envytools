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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "buffer.h"
#include "demmt.h"
#include "drm.h"
#include "macro.h"
#include "nvrm.h"
#include "object_state.h"
#include "util.h"
#include "log.h"

int dump_raw_ioctl_data = 0;
int dump_decoded_ioctl_data = 1;
int dump_tsc = 1;
int dump_tic = 1;
int dump_vp = 1;
int dump_fp = 1;
int dump_gp = 1;
int dump_cp = 1;
int dump_tep = 1;
int dump_tcp = 1;
int dump_buffer_usage = 1;
int decode_pb = 1;
int dump_sys_mmap = 1;
int dump_sys_munmap = 1;
int dump_sys_mremap = 1;
int dump_sys_open = 1;
int dump_msg = 1;
int dump_sys_write = 1;
int print_gpu_addresses = 0;

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

int chipset;
int ib_supported;
int indent_logs = 0;
int is_nouveau = 0;
const struct envy_colors *colors = NULL;
int dump_memory_writes = 1;
int dump_memory_reads = 1;
int info = 1;

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

static void usage()
{
	fflush(stdout);
	fprintf(stderr, "Usage: demmt [OPTION]\n"
			"Decodes binary trace files generated by Valgrind MMT. Reads standard input or file passed by -l.\n\n"
			"  -l file\t\tuse \"file\" as input\n"
			"         \t\t- it can be compressed by gzip, bzip2 or xz\n"
			"         \t\t- demmt extracts chipset version from characters following \"nv\"\n"
			"  -m 'chipset'\t\tset chipset version (required, but see -l)\n"
			"  -q\t\t\t(quiet) print only the most important data (= -d all -e pb,shader,macro,tsc,tic,cp,classes=all,buffer-usage,nvrm-class-desc)\n"
			"  -c 0/1\t\tdisable/enable colors (default: 1)\n"
			"  -g 0/1\t\t= -d/-e gpu-addr (default: 0)\n"
			"  -o 0/1\t\t= -d/-e ioctl-raw (default: 0)\n"
			"  -r 0/1\t\t= -d/-e macro-rt-verbose (default: 0)\n"
			"  -i 0/1\t\tdisable/enable log indentation (default: 0)\n"
			"  -f\t\t\tfind possible pushbuf pointers (IB / USER)\n"
			"  -n id[,offset]\tset pushbuf pointer to \"id\" and optionally offset within this buffer to \"offset\"\n"
			"  -a\t\t\t= -d classes=all\n"
			"  -x\t\t\tforce pushbuf decoding even without pushbuf pointer\n"
			"\n"
			"  -d msg_type1[,msg_type2[,msg_type3....]] - disable messages\n"
			"  -e msg_type1[,msg_type2[,msg_type3....]] - enable messages\n"
			"     message types:\n"
			"     - write - memory write\n"
			"     - read - memory read\n"
			"     - gpu-addr - gpu address\n"
			"     - mem = read,write\n"
			"     - pb - push buffer\n"
			"     - class=[all,0x...] - class decoder\n"
			"     - tsc - texture sampler control block\n"
			"     - tic - texture image control block\n"
			"     - vp - vertex program\n"
			"     - fp - fragment program\n"
			"     - gp - geometry program\n"
			"     - cp - compute program\n"
			"     - tep\n"
			"     - tcp\n"
			"     - shader = vp,fp,gp,tep,tcp\n"
			"     - macro-rt-verbose - verbose macro interpreter\n"
			"     - macro-rt - macro interpreter \n"
			"     - macro-dis - macro disasm\n"
			"     - macro = macro-rt,macro-dis\n"
			"     - sys_mmap\n"
			"     - sys_munmap\n"
			"     - sys_mremap\n"
			"     - sys_open\n"
			"     - sys_write\n"
			"     - sys = sys_mmap,sys_munmap,sys_mremap,sys_open,sys_write,ioctl-desc\n"
			"     - ioctl-raw - raw ioctl data\n"
			"     - ioctl-desc - decoded ioctl\n"
			"     - ioctl = ioctl-raw,ioctl-desc\n"
			"     - nvrm-ioctl=[all,name] name=create,call,host_map,etc...\n"
			"     - nvrm-mthd=[all,0x...] - method call\n"
			"     - nvrm-handle-desc - handle description\n"
			"     - nvrm-class-desc - class description\n"
			"     - nvrm-unk-0-fields - unk zero fields\n"
			"     - nvrm = nvrm-ioctl=all,nvrm-mthd=all,nvrm-handle-desc,nvrm-class-desc\n"
			"     - buffer-usage\n"
			"     - msg - textual valgrind message\n"
			"     - info - various informations\n"
			"     - all - everything above\n"
			"\n");
	exit(1);
}

#define DEF_INT_FUN(name, var) static void _filter_##name(int en) { var = en; }

DEF_INT_FUN(info, info);
DEF_INT_FUN(write, dump_memory_writes);
DEF_INT_FUN(read, dump_memory_reads);
DEF_INT_FUN(gpu_addr, print_gpu_addresses);
DEF_INT_FUN(ioctl_raw, dump_raw_ioctl_data);
DEF_INT_FUN(ioctl_desc, dump_decoded_ioctl_data);
DEF_INT_FUN(tsc, dump_tsc);
DEF_INT_FUN(tic, dump_tic);
DEF_INT_FUN(vp, dump_vp);
DEF_INT_FUN(fp, dump_fp);
DEF_INT_FUN(gp, dump_gp);
DEF_INT_FUN(cp, dump_cp);
DEF_INT_FUN(tep, dump_tep);
DEF_INT_FUN(tcp, dump_tcp);
DEF_INT_FUN(macro_rt_verbose, macro_rt_verbose);
DEF_INT_FUN(macro_rt, macro_rt);
DEF_INT_FUN(macro_dis_enabled, macro_dis_enabled);
DEF_INT_FUN(buffer_usage, dump_buffer_usage);
DEF_INT_FUN(decode_pb, decode_pb);
DEF_INT_FUN(sys_mmap, dump_sys_mmap);
DEF_INT_FUN(sys_munmap, dump_sys_munmap);
DEF_INT_FUN(sys_mremap, dump_sys_mremap);
DEF_INT_FUN(sys_open, dump_sys_open);
DEF_INT_FUN(sys_write, dump_sys_write);
DEF_INT_FUN(msg, dump_msg);
DEF_INT_FUN(nvrm_describe_handles, nvrm_describe_handles);
DEF_INT_FUN(nvrm_describe_classes, nvrm_describe_classes);
DEF_INT_FUN(nvrm_show_unk_zero_fields, nvrm_show_unk_zero_fields);

static void _filter_class(const char *token, int en)
{
	struct gpu_object_decoder *o;
	uint32_t class_ = strtoul(token, NULL, 16);

	for (o = obj_decoders; o->class_ != 0; o++)
		if (o->class_ == class_)
		{
			o->disabled = !en;
			break;
		}
}
static void _filter_all_classes(int en)
{
	struct gpu_object_decoder *o;

	for (o = obj_decoders; o->class_ != 0; o++)
		o->disabled = !en;
}

static void _filter_nvrm_mthd(const char *token, int en)
{
	uint32_t mthd = strtoul(token, NULL, 16);
	int i;

	for (i = 0; i < nvrm_mthds_cnt; ++i)
		if (nvrm_mthds[i].mthd == mthd)
		{
			nvrm_mthds[i].disabled = !en;
			break;
		}
}

static void _filter_all_nvrm_mthds(int en)
{
	int i;
	for (i = 0; i < nvrm_mthds_cnt; ++i)
		nvrm_mthds[i].disabled = !en;
}

static void _filter_nvrm_ioctl(const char *token, int en)
{
	int i;
	for (i = 0; i < nvrm_ioctls_cnt; ++i)
		if (strcasecmp(nvrm_ioctls[i].name + strlen("NVRM_IOCTL_"), token) == 0)
		{
			nvrm_ioctls[i].disabled = !en;
			break;
		}
}

static void _filter_all_nvrm_ioctls(int en)
{
	int i;
	for (i = 0; i < nvrm_ioctls_cnt; ++i)
		nvrm_ioctls[i].disabled = !en;
}

static void handle_filter_opt(const char *_arg, int en)
{
	char *arg = strdup(_arg);
	char *tokens = arg;

	while (tokens)
	{
		char *token = strsep(&tokens, ",");
		if (strcmp(token, "write") == 0)
			_filter_write(en);
		else if (strcmp(token, "read") == 0)
			_filter_read(en);
		else if (strcmp(token, "gpu-addr") == 0)
			_filter_gpu_addr(en);
		else if (strcmp(token, "mem") == 0)
		{
			_filter_write(en);
			_filter_read(en);
		}
		else if (strcmp(token, "ioctl-raw") == 0)
			_filter_ioctl_raw(en);
		else if (strcmp(token, "ioctl-desc") == 0)
			_filter_ioctl_desc(en);
		else if (strcmp(token, "ioctl") == 0)
		{
			if (!en)
				_filter_ioctl_raw(en);
			_filter_ioctl_desc(en);
		}
		else if (strcmp(token, "tsc") == 0)
			_filter_tsc(en);
		else if (strcmp(token, "tic") == 0)
			_filter_tic(en);
		else if (strcmp(token, "vp") == 0)
			_filter_vp(en);
		else if (strcmp(token, "fp") == 0)
			_filter_fp(en);
		else if (strcmp(token, "gp") == 0)
			_filter_gp(en);
		else if (strcmp(token, "cp") == 0)
			_filter_cp(en);
		else if (strcmp(token, "tep") == 0)
			_filter_tep(en);
		else if (strcmp(token, "tcp") == 0)
			_filter_tcp(en);
		else if (strcmp(token, "shader") == 0)
		{
			_filter_vp(en);
			_filter_fp(en);
			_filter_gp(en);
			_filter_tcp(en);
			_filter_tep(en);
		}
		else if (strcmp(token, "macro-rt-verbose") == 0)
			_filter_macro_rt_verbose(en);
		else if (strcmp(token, "macro-rt") == 0)
			_filter_macro_rt(en);
		else if (strcmp(token, "macro-dis") == 0)
			_filter_macro_dis_enabled(en);
		else if (strcmp(token, "macro") == 0)
		{
			if (!en)
				_filter_macro_rt_verbose(en);
			_filter_macro_rt(en);
			_filter_macro_dis_enabled(en);
		}
		else if (strcmp(token, "buffer-usage") == 0)
			_filter_buffer_usage(en);
		else if (strncmp(token, "class=", 6) == 0)
		{
			if (strcmp(token + 6, "all") == 0)
				_filter_all_classes(en);
			else
				_filter_class(token + 6, en);
		}
		else if (strcmp(token, "pb") == 0)
			_filter_decode_pb(en);
		else if (strcmp(token, "sys_mmap") == 0)
			_filter_sys_mmap(en);
		else if (strcmp(token, "sys_munmap") == 0)
			_filter_sys_munmap(en);
		else if (strcmp(token, "sys_mremap") == 0)
			_filter_sys_mremap(en);
		else if (strcmp(token, "sys_open") == 0)
			_filter_sys_open(en);
		else if (strcmp(token, "msg") == 0)
			_filter_msg(en);
		else if (strcmp(token, "sys_write") == 0)
			_filter_sys_write(en);
		else if (strcmp(token, "sys") == 0)
		{
			_filter_sys_mmap(en);
			_filter_sys_munmap(en);
			_filter_sys_mremap(en);
			_filter_sys_open(en);
			_filter_sys_write(en);

			if (!en)
				_filter_ioctl_raw(en);
			_filter_ioctl_desc(en);
		}
		else if (strcmp(token, "nvrm-handle-desc") == 0)
			_filter_nvrm_describe_handles(en);
		else if (strcmp(token, "nvrm-class-desc") == 0)
			_filter_nvrm_describe_classes(en);
		else if (strncmp(token, "nvrm-mthd=", 10) == 0)
		{
			if (strcmp(token + 10, "all") == 0)
				_filter_all_nvrm_mthds(en);
			else
				_filter_nvrm_mthd(token + 10, en);
			if (en)
			{
				_filter_nvrm_ioctl("call", en);
				_filter_ioctl_desc(en);
			}
		}
		else if (strcmp(token, "nvrm-unk-0-fields") == 0)
			_filter_nvrm_show_unk_zero_fields(en);
		else if (strncmp(token, "nvrm-ioctl=", 11) == 0)
		{
			if (strcmp(token + 11, "all") == 0)
				_filter_all_nvrm_ioctls(en);
			else
				_filter_nvrm_ioctl(token + 11, en);
			if (en)
				_filter_ioctl_desc(en);
		}
		else if (strcmp(token, "nvrm") == 0)
		{
			if (en)
				_filter_ioctl_desc(en);
			_filter_all_nvrm_ioctls(en);
			_filter_all_nvrm_mthds(en);
			_filter_nvrm_describe_handles(en);
			_filter_nvrm_describe_classes(en);
		}
		else if (strcmp(token, "info") == 0)
			_filter_info(en);
		else if (strcmp(token, "all") == 0)
		{
			_filter_info(en);
			_filter_write(en);
			_filter_read(en);
			_filter_gpu_addr(en);
			_filter_ioctl_raw(en);
			_filter_ioctl_desc(en);
			_filter_tsc(en);
			_filter_tic(en);
			_filter_vp(en);
			_filter_fp(en);
			_filter_gp(en);
			_filter_cp(en);
			_filter_tep(en);
			_filter_tcp(en);
			_filter_macro_rt_verbose(en);
			_filter_macro_rt(en);
			_filter_macro_dis_enabled(en);
			_filter_buffer_usage(en);
			_filter_all_classes(en);
			_filter_decode_pb(en);
			_filter_sys_mmap(en);
			_filter_sys_munmap(en);
			_filter_sys_mremap(en);
			_filter_sys_open(en);
			_filter_msg(en);
			_filter_sys_write(en);
			_filter_nvrm_describe_handles(en);
			_filter_nvrm_describe_classes(en);
			_filter_all_nvrm_mthds(en);
			_filter_nvrm_show_unk_zero_fields(en);
			_filter_all_nvrm_ioctls(en);
		}
		else
		{
			fprintf(stderr, "unknown token: %s\n", token);
			fflush(stderr);
			exit(1);
		}
	}

	free(arg);
}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	if (argc < 2)
		usage();

	int c;
	while ((c = getopt (argc, argv, "m:n:o:g:fqac:l:i:xr:he:d:")) != -1)
	{
		switch (c)
		{
			case 'm':
			{
				char *chip = optarg;
				if (strncasecmp(chip, "NV", 2) == 0)
					chip += 2;
				chipset = strtoul(chip, NULL, 16);
				break;
			}
			case 'n':
			{
				char *endptr;
				pb_pointer_buffer = strtoul(optarg, &endptr, 10);
				if (endptr && endptr[0] == ',')
					pb_pointer_offset = strtoul(endptr + 1, NULL, 0);
				break;
			}
			case 'o':
				if (optarg[0] == '1')
					_filter_ioctl_raw(1);
				else if (optarg[0] == '0')
					_filter_ioctl_raw(0);
				else
				{
					fprintf(stderr, "-o accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'g':
				if (optarg[0] == '1')
					_filter_gpu_addr(1);
				else if (optarg[0] == '0')
					_filter_gpu_addr(0);
				else
				{
					fprintf(stderr, "-g accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'f':
				find_pb_pointer = 1;
				handle_filter_opt("all", 0);
				break;
			case 'q':
				handle_filter_opt("all", 0);
				_filter_decode_pb(1);
				_filter_tsc(1);
				_filter_tic(1);
				_filter_cp(1);
				handle_filter_opt("shader", 1);
				handle_filter_opt("macro", 1);
				_filter_buffer_usage(1);
				_filter_all_classes(1);
				_filter_nvrm_describe_classes(1);
				break;
			case 'a':
				_filter_all_classes(0);
				break;
			case 'c':
				if (optarg[0] == '1')
					colors = &envy_def_colors;
				else if (optarg[0] == '0')
					colors = &envy_null_colors;
				else
				{
					fprintf(stderr, "-c accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'l':
			{
				filename = strdup(optarg);
				const char *base = basename(filename);
				if (chipset == 0 && strncasecmp(base, "nv", 2) == 0)
					chipset = strtoul(base + 2, NULL, 16);
				break;
			}
			case 'i':
				if (optarg[0] == '1')
					indent_logs = 1;
				else if (optarg[0] == '0')
					indent_logs = 0;
				else
				{
					fprintf(stderr, "-i accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'x':
				force_pushbuf_decoding = 1;
				break;
			case 'r':
				if (optarg[0] == '1')
					_filter_macro_rt_verbose(1);
				else if (optarg[0] == '0')
					_filter_macro_rt_verbose(0);
				else
				{
					fprintf(stderr, "-r accepts only 0 and 1\n");
					exit(1);
				}
				break;
			case 'h':
			case '?':
				usage();
				break;
			case 'd':
				handle_filter_opt(optarg, 0);
				break;
			case 'e':
				handle_filter_opt(optarg, 1);
				break;
		}
	}

	if (chipset == 0)
		usage();
	ib_supported = chipset >= 0x80 || chipset == 0x50;

	if (!colors)
		colors = &envy_def_colors;

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
