#ifndef DEMMT_CONFIG_H
#define DEMMT_CONFIG_H

#include "colors.h"

extern const struct envy_colors *colors;

extern int chipset;
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
extern int pager_enabled;
extern int dump_memory_writes;
extern int dump_memory_reads;
extern int dump_object_tree_on_create_destroy;

char *read_opts(int argc, char *argv[]);

#endif
