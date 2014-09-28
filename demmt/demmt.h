#ifndef DEMMT_H
#define DEMMT_H

#include "rnn.h"
#include "rnndec.h"
#include "colors.h"

extern const struct envy_colors *colors;

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern struct rnndb *rnndb_nvrm_object;
extern struct rnndeccontext *g80_texture_ctx;
extern struct rnndeccontext *gf100_shaders_ctx;
extern struct rnndomain *tsc_domain;
extern struct rnndomain *tic_domain;
extern struct rnndomain *gf100_vp_header_domain, *gf100_fp_header_domain,
	*gf100_gp_header_domain, *gf100_tcp_header_domain, *gf100_tep_header_domain;

extern int chipset;
extern int ib_supported;
extern int is_nouveau;
extern int dump_raw_ioctl_data;
extern int dump_decoded_ioctl_data;
extern int dump_tsc;
extern int dump_tic;
extern int dump_vp;
extern int dump_fp;
extern int dump_gp;
extern int dump_cp;
extern int dump_tep;
extern int dump_tcp;
extern int dump_buffer_usage;
extern int decode_pb;
extern int dump_sys_mmap;
extern int dump_sys_munmap;
extern int dump_sys_mremap;
extern int dump_sys_open;
extern int dump_msg;
extern int dump_sys_write;
extern int info;
extern int print_gpu_addresses;
extern int dump_memory_writes;
extern int dump_memory_reads;

#endif
