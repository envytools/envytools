#ifndef DEMMT_H
#define DEMMT_H

#include "demmt_pushbuf.h"

#define MMT_DEBUG 0

#define mmt_debug(fmt, ...)        do { if (MMT_DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define mmt_log(fmt, ...)          do { fprintf(stdout, "%64s" fmt, " ", __VA_ARGS__); } while (0)
#define mmt_log_cont(fmt, ...)     do { fprintf(stdout, fmt, __VA_ARGS__); } while (0)
#define mmt_error(fmt, ...)        do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)

struct buffer
{
	unsigned char *data;
	int length;
	uint64_t mmap_offset;
	uint64_t cpu_start;
	uint64_t data1;
	uint64_t data2;
	uint64_t gpu_start;
	enum BUFTYPE { PUSH, IB } type;
	union
	{
		struct pushbuf_decode_state pushbuf;
		struct ib_decode_state ib;
	} state;
};

#define MAX_ID 1024
extern struct buffer *buffers[MAX_ID];

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern int chipset;
extern int guess_invalid_pushbuf;
extern int m2mf_hack_enabled;

#endif
