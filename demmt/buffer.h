#ifndef DEMMT_BUFFER_H
#define DEMMT_BUFFER_H

#include <stdint.h>
#include "mmt_bin_decode.h"
#include "pushbuf.h"
#include "region.h"

struct unk_map
{
	uint32_t data1;
	uint32_t data2;
	uint64_t mmap_offset;
	struct unk_map *next;
};
extern struct unk_map *unk_maps;

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

extern int find_pb_pointer;
extern uint32_t pb_pointer_buffer;
extern uint32_t pb_pointer_offset;
extern int force_pushbuf_decoding;

void buffer_register_write(struct buffer *buf, uint32_t offset, uint8_t len, const void *data);
void register_gpu_only_buffer(uint64_t gpu_start, int len, uint64_t mmap_offset, uint64_t data1, uint64_t data2);
void buffer_free(struct buffer *buf);

void buffer_register_mmt_read(struct mmt_read *r);
void buffer_register_mmt_write(struct mmt_write *w);

void buffer_mmap(uint32_t id, uint64_t cpu_start, uint64_t len, uint64_t mmap_offset,
		const uint64_t *data1, const uint64_t *data2);
void buffer_munmap(uint32_t id);
void buffer_mremap(struct mmt_mremap *mm);
void buffer_ioctl_pre(int print_raw);
void buffer_flush();

#endif
