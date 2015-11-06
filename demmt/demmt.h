#ifndef DEMMT_H
#define DEMMT_H

#include "rnn.h"
#include "rnndec.h"
#include <stdnoreturn.h>

#define MAX_USAGES 32

enum mmt_fd_type { FDUNK, FDNVIDIA, FDDRM, FDFGLRX };
enum mmt_fd_type demmt_get_fdtype(int fd);

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern struct rnndb *rnndb_nvrm_object;
extern struct rnndb *rnndb_g80_texture;
extern struct rnndeccontext *gf100_shaders_ctx;
extern struct rnndomain *tsc_domain;
extern struct rnndomain *tic_domain;
extern struct rnndomain *gf100_sp_header_domain, *gf100_fp_header_domain;
extern struct rnndomain *gk104_cp_header_domain;
extern int mmt_sync_fd;

uint64_t roundup_to_pagesize(uint64_t sz);

struct bitfield_desc
{
	uint32_t mask;
	const char *name;
};

void decode_bitfield(uint32_t data, struct bitfield_desc *bfdesc);

void decode_mmap_prot(uint32_t prot);
void decode_mmap_flags(uint32_t flags);
static inline noreturn void demmt_abort() { exit(1); }

#endif
