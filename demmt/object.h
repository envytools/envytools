#ifndef DEMMT_OBJECT_H
#define DEMMT_OBJECT_H

// internal to object*.c

#include "buffer.h"
#include "dis.h"
#include "log.h"
#include "pushbuf.h"

extern const struct disisa *isa_macro;
extern const struct disisa *isa_g80;
extern const struct disisa *isa_gf100;
extern const struct disisa *isa_gk110;
extern const struct disisa *isa_gm107;

void decode_g80_2d_init(struct gpu_object *);
void decode_g80_2d_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_g80_2d_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_g80_3d_init(struct gpu_object *);
void decode_g80_3d_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_g80_3d_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_g80_m2mf_init(struct gpu_object *);
void decode_g80_m2mf_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_g80_m2mf_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_gf100_2d_init(struct gpu_object *);
void decode_gf100_2d_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_gf100_3d_init(struct gpu_object *);
void decode_gf100_3d_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_gf100_3d_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);
void gf100_3d_disassemble(uint8_t *data, struct region *reg,
		uint32_t start_id, const struct disisa *isa, struct varinfo *var);

void decode_gf100_m2mf_init(struct gpu_object *);
void decode_gf100_m2mf_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_gf100_m2mf_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_gk104_3d_init(struct gpu_object *);
void decode_gk104_3d_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_gk104_3d_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_gk104_compute_init(struct gpu_object *);
void decode_gk104_compute_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_gk104_compute_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_gk104_copy_init(struct gpu_object *);
void decode_gk104_copy_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);

void decode_gk104_p2mf_init(struct gpu_object *);
void decode_gk104_p2mf_terse(struct gpu_object *, struct pushbuf_decode_state *pstate);
void decode_gk104_p2mf_verbose(struct gpu_object *, struct pushbuf_decode_state *pstate);

struct rnndeccontext *create_g80_texture_ctx(struct gpu_object *obj);

void decode_tsc(struct rnndeccontext *texture_ctx, uint32_t tsc, int idx, uint32_t *data);
void decode_tic(struct rnndeccontext *texture_ctx, uint32_t tic, int idx, uint32_t *data);

struct addr_n_buf
{
	uint64_t address;

	struct gpu_mapping *gpu_mapping;
	struct gpu_mapping *prev_gpu_mapping;
};

struct nv1_graph
{
	struct addr_n_buf notify;
};

struct subchan
{
	struct addr_n_buf semaphore;
};

struct gk104_upload
{
	struct addr_n_buf dst;
	struct addr_n_buf query;
};

struct mthd2addr
{
	uint32_t high, low;
	struct addr_n_buf *buf;
	int length, stride;
	int check_offset;
};

static inline struct mthd2addr *m2a_set1(struct mthd2addr *addr, uint32_t high,
		uint32_t low, struct addr_n_buf *buf)
{
	addr->high = high;
	addr->low = low;
	addr->buf = buf;
	addr->length = 0;
	addr->stride = 0;
	addr->check_offset = 0;
	return addr;
}

static inline struct mthd2addr *m2a_setN(struct mthd2addr *addr, uint32_t high,
		uint32_t low, struct addr_n_buf *buf, int length, int stride)
{
	addr->high = high;
	addr->low = low;
	addr->buf = buf;
	addr->length = length;
	addr->stride = stride;
	addr->check_offset = 0;
	return addr;
}

int check_addresses_terse(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses);
int check_addresses_verbose(struct pushbuf_decode_state *pstate, struct mthd2addr *addresses);

#endif
