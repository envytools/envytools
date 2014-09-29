#ifndef DEMMT_OBJECT_H
#define DEMMT_OBJECT_H

// internal to object*.c

#include "buffer.h"
#include "dis.h"
#include "log.h"
#include "pushbuf.h"

extern const struct disisa *isa_gf100;
extern const struct disisa *isa_gk110;
extern const struct disisa *isa_gm107;

void decode_g80_2d_terse(struct pushbuf_decode_state *pstate);
void decode_g80_2d_verbose(struct pushbuf_decode_state *pstate);
void decode_g80_3d_terse(struct pushbuf_decode_state *pstate);
void decode_g80_3d_verbose(struct pushbuf_decode_state *pstate);
void decode_g80_m2mf_terse(struct pushbuf_decode_state *pstate);
void decode_gf100_2d_terse(struct pushbuf_decode_state *pstate);
void decode_gf100_3d_terse(struct pushbuf_decode_state *pstate);
void decode_gf100_3d_verbose(struct pushbuf_decode_state *pstate);
void decode_gf100_m2mf_terse(struct pushbuf_decode_state *pstate);
void decode_gf100_m2mf_verbose(struct pushbuf_decode_state *pstate);
void decode_gk104_compute_terse(struct pushbuf_decode_state *pstate);
void decode_gk104_compute_verbose(struct pushbuf_decode_state *pstate);
void decode_gk104_copy_terse(struct pushbuf_decode_state *pstate);
void decode_gk104_p2mf_terse(struct pushbuf_decode_state *pstate);
void decode_gk104_p2mf_verbose(struct pushbuf_decode_state *pstate);

void decode_tsc(uint32_t tsc, int idx, uint32_t *data);
void decode_tic(uint32_t tic, int idx, uint32_t *data);

struct addr_n_buf
{
	uint64_t address;
	struct buffer *buffer;
	struct buffer *prev_buffer;
};

struct nv1_graph
{
	struct addr_n_buf notify;
};

struct subchan
{
	struct addr_n_buf semaphore;
};

struct mthd2addr
{
	uint32_t high, low;
	struct addr_n_buf *buf;
	int length, stride;
};

int check_addresses_terse(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses);
int check_addresses_verbose(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses);

struct buffer *find_buffer_by_gpu_address(uint64_t addr);

#endif
