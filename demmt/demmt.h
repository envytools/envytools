#ifndef DEMMT_H
#define DEMMT_H

#include "rnn.h"
#include "rnndec.h"

extern struct rnndomain *domain;
extern struct rnndb *rnndb;
extern struct rnndb *rnndb_nvrm_object;
extern struct rnndeccontext *g80_texture_ctx;
extern struct rnndeccontext *gf100_shaders_ctx;
extern struct rnndomain *tsc_domain;
extern struct rnndomain *tic_domain;
extern struct rnndomain *gf100_vp_header_domain, *gf100_fp_header_domain,
	*gf100_gp_header_domain, *gf100_tcp_header_domain, *gf100_tep_header_domain;

#endif
