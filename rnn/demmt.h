#ifndef DEMMT_H
#define DEMMT_H

#include "demmt_pushbuf.h"
#include "colors.h"

#define MMT_DEBUG 0
extern int find_pb_pointer;
extern int quiet;
extern int disassemble_shaders;
extern int indent_logs;
extern const struct envy_colors *colors;

#include <stdio.h>
#define mmt_debug(fmt, ...)        do { if (MMT_DEBUG)                  fprintf(stdout, "DBG: " fmt, __VA_ARGS__); } while (0)
#define mmt_debug_cont(fmt, ...)   do { if (MMT_DEBUG)                  fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_log(fmt, ...)          do { if (!find_pb_pointer && !quiet) { if (indent_logs) fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); else fprintf(stdout, "LOG: " fmt, __VA_ARGS__); } } while (0)
#define mmt_log_cont(fmt, ...)     do { if (!find_pb_pointer && !quiet) fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_error(fmt, ...)        do { fprintf(stdout, "ERROR: " fmt, __VA_ARGS__); } while (0)

struct region
{
	struct region *prev;
	uint32_t start;
	uint32_t end;
	struct region *next;
};

struct buffer
{
	int id;
	unsigned char *data;
	int length;
	uint64_t mmap_offset;
	uint64_t cpu_start;
	uint64_t data1;
	uint64_t data2;
	uint64_t gpu_start;
	enum BUFTYPE { PUSH, IB, USER } type;
	union
	{
		struct pushbuf_decode_state pushbuf;
		struct ib_decode_state ib;
		struct user_decode_state user;
	} state;
	struct region *written_regions;
	struct region *written_region_last;
	struct buffer *prev, *next;
};

#define MAX_ID 16 * 1024
extern struct buffer *buffers[MAX_ID];
extern struct buffer *buffers_list;

extern struct buffer *gpu_only_buffers_list;

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern struct rnndeccontext *nv50_texture_ctx;
extern struct rnndeccontext *nvc0_shaders_ctx;
extern struct rnndomain *tsc_domain;
extern struct rnndomain *tic_domain;
extern struct rnndomain *nvc0_vp_header_domain, *nvc0_fp_header_domain, *nvc0_gp_header_domain, *nvc0_tcp_header_domain, *nvc0_tep_header_domain;

extern int chipset;
extern int ib_supported;
extern int guess_invalid_pushbuf;
extern int is_nouveau;

void buffer_register_write(struct buffer *buf, uint32_t offset, uint8_t len, const void *data);
void register_gpu_only_buffer(uint64_t gpu_start, int len, uint64_t mmap_offset, uint64_t data1, uint64_t data2);

#endif
