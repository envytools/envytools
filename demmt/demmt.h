#ifndef DEMMT_H
#define DEMMT_H

#include "rnn.h"
#include "rnndec.h"

#define MAX_ID 16 * 1024
#define MAX_USAGES 32

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern struct rnndb *rnndb_nvrm_object;
extern struct rnndb *rnndb_g80_texture;
extern struct rnndeccontext *gf100_shaders_ctx;
extern struct rnndomain *tsc_domain;
extern struct rnndomain *tic_domain;
extern struct rnndomain *gf100_vp_header_domain, *gf100_fp_header_domain,
	*gf100_gp_header_domain, *gf100_tcp_header_domain, *gf100_tep_header_domain;
extern int mmt_sync_fd;

uint64_t roundup_to_pagesize(uint64_t sz);

#endif
