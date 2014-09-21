#ifndef DEMMT_H
#define DEMMT_H

#include "demmt_pushbuf.h"
#include "demmt_region.h"
#include "colors.h"

#define MMT_DEBUG 0
extern int find_pb_pointer;
extern int indent_logs;
extern const struct envy_colors *colors;

struct unk_map
{
	uint32_t data1;
	uint32_t data2;
	uint64_t mmap_offset;
	struct unk_map *next;
};
extern struct unk_map *unk_maps;

extern uint32_t pb_pointer_buffer;
extern uint32_t pb_pointer_offset;

#include <stdio.h>
#define mmt_debug(fmt, ...)        do { if (MMT_DEBUG)        fprintf(stdout, "DBG: " fmt, __VA_ARGS__); } while (0)
#define mmt_debug_cont(fmt, ...)   do { if (MMT_DEBUG)        fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_log(fmt, ...)          do { if (indent_logs) fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); else fprintf(stdout, "LOG: " fmt, __VA_ARGS__); } while (0)
#define mmt_log_cont(fmt, ...)     do { fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_log_cont_nl()          do { fprintf(stdout, "\n"); } while (0)
#define mmt_error(fmt, ...)        do { fprintf(stdout, "ERROR: " fmt, __VA_ARGS__); } while (0)

struct buffer
{
	int id;
	unsigned char *data;
	uint64_t length;
	uint64_t mmap_offset;
	uint64_t cpu_start;
	uint64_t data1;
	uint64_t data2;
	uint64_t gpu_start;
	enum BUFTYPE { PUSH = 1, IB = 2, USER = 4 } type;
	uint32_t ib_offset;
	union
	{
		struct pushbuf_decode_state pushbuf;
		struct ib_decode_state ib;
		struct user_decode_state user;
	} state;
	struct regions written_regions;
	struct buffer *prev, *next;

#define MAX_USAGES 32
	struct
	{
		char *desc;
		uint64_t address;
	}
	usage[MAX_USAGES];
};

#define MAX_ID 16 * 1024
extern struct buffer *buffers[MAX_ID];
extern struct buffer *buffers_list;

extern struct buffer *gpu_only_buffers_list;

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
extern int force_pushbuf_decoding;
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

void buffer_dump(struct buffer *buf);
void buffer_register_write(struct buffer *buf, uint32_t offset, uint8_t len, const void *data);
void register_gpu_only_buffer(uint64_t gpu_start, int len, uint64_t mmap_offset, uint64_t data1, uint64_t data2);
void buffer_remove(struct buffer *buf);
void buffer_free(struct buffer *buf);

void __demmt_mmap(uint32_t id, uint64_t cpu_start, uint64_t len, uint64_t mmap_offset,
		const uint64_t *data1, const uint64_t *data2);

#endif
