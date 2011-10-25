/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ED2I_PRE_H
#define ED2I_PRE_H

#include "ed2i.h"
#include "ed2_misc.h"

struct ed2ip_feature {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **implies;
	int impliesnum;
	int impliesmax;
	char **conflicts;
	int conflictsnum;
	int conflictsmax;
	struct ed2_loc loc;
};

struct ed2ip_variant {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **features;
	int featuresnum;
	int featuresmax;
	struct ed2_loc loc;
};

struct ed2ip_modeset {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	int isoptional;
	struct ed2ip_mode **modes;
	int modesnum;
	int modesmax;
	struct ed2_loc loc;
};

struct ed2ip_mode {
	char **names;
	int namesnum;
	int namesmax;
	char *description;
	char **features;
	int featuresnum;
	int featuresmax;
	int isdefault;
	struct ed2_loc loc;
};

struct ed2ip_bitchunk {
	char *name;
	int from;
	int to;
};

struct ed2ip_bitfield {
	struct ed2ip_bitchunk **chunks;
	int chunksnum;
	int chunksmax;
};

struct ed2ip_caseval {
	uint64_t val1;
	uint64_t val2;
	uint64_t mask;;
	char *str;
};

struct ed2ip_casepart {
	enum ed2ip_cptype {
		ED2IP_CP_BITFIELD,
		ED2IP_CP_FEATURE,
		ED2IP_CP_MODE,
	} type;
	struct ed2ip_bitfield *bf;
	struct ed2ip_caseval **vals;
	int valsnum;
	int valsmax;
	char **names;	// modes or features
	int namesnum;
	int namesmax;
};

struct ed2ip_case {
	struct ed2ip_casepart **parts;
	int partsnum;
	int partsmax;
};

struct ed2ip_enumval {
	char *name;
	int isdefault;
};

struct ed2ip_opfield {
	char *name;
	int len;
	struct ed2ip_enumval **enumvals;
	int enumvalsnum;
	int enumvalsmax;
	int hasdef;
	uint64_t defval;
};

struct ed2ip_insn {
	enum ed2ip_insntype {
		ED2IP_INSN_INSN,
	} type;
	char *iname;
};

struct ed2ip_seq {
	char *name;
	enum ed2ip_seqtype {
		ED2IP_SEQ_SEQ,
		ED2IP_SEQ_SWITCH,
		ED2IP_SEQ_READ,
		ED2IP_SEQ_INSN_INLINE,
		ED2IP_SEQ_INSN_REF,
		ED2IP_SEQ_CALL,
		ED2IP_SEQ_LET_COPY,
		ED2IP_SEQ_LET_CONST,
	} type;
	int broken;
	struct ed2ip_seq **seqs;
	int seqsnum;
	int seqsmax;
	struct ed2ip_case **cases;
	int casesnum;
	int casesmax;
	struct ed2ip_insn *insn_inline;
	enum ed2i_endian read_endian;
	char *call_seq;
	struct ed2ip_bitfield *bf_dst;
	struct ed2ip_bitfield *bf_src;
	uint64_t const_src;
	char *const_str;
	char *insn_ref;
};

struct ed2ip_isa {
	struct ed2ip_feature **features;
	int featuresnum;
	int featuresmax;
	struct ed2ip_variant **variants;
	int variantsnum;
	int variantsmax;
	struct ed2ip_modeset **modesets;
	int modesetsnum;
	int modesetsmax;
	struct ed2ip_opfield **opfields;
	int opfieldsnum;
	int opfieldsmax;
	struct ed2ip_seq **seqs;
	int seqsnum;
	int seqsmax;
	int broken;
};

void ed2ip_del_feature(struct ed2ip_feature *f);
void ed2ip_del_variant(struct ed2ip_variant *v);
void ed2ip_del_modeset(struct ed2ip_modeset *ms);
void ed2ip_del_mode(struct ed2ip_mode *m);
void ed2ip_del_enumval(struct ed2ip_enumval *ev);
void ed2ip_del_opfield(struct ed2ip_opfield *opf);
void ed2ip_del_insn(struct ed2ip_insn *in);
void ed2ip_del_seq(struct ed2ip_seq *s);
void ed2ip_del_case(struct ed2ip_case *c);
void ed2ip_del_casepart(struct ed2ip_casepart *cp);
void ed2ip_del_caseval(struct ed2ip_caseval *cv);
void ed2ip_del_bitfield(struct ed2ip_bitfield *bf);
void ed2ip_del_bitchunk(struct ed2ip_bitchunk *bc);
void ed2ip_del_isa(struct ed2ip_isa *isa);

struct ed2i_isa *ed2ip_transform(struct ed2ip_isa *isa);

#endif
