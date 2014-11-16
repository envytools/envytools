#ifndef NVRM_DECODE_H
#define NVRM_DECODE_H

#include "config.h"
#include "log.h"
#include "mmt_bin_decode.h"
#include "mmt_bin_decode_nvidia.h"
#include "nvrm_ioctl.h"

extern int nvrm_show_unk_zero_fields;

int decode_nvrm_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc);
int decode_nvrm_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc);
void decode_nvrm_ioctl_query(struct nvrm_ioctl_query *s, struct mmt_memory_dump *args, int argc);
void decode_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc);

struct mmt_buf *find_ptr(uint64_t ptr, struct mmt_memory_dump *args, int argc);
const char *nvrm_status(uint32_t status);
const char *nvrm_get_class_name(uint32_t cls);

void dump_mmt_buf_as_text(struct mmt_buf *buf, const char *pfx);
void dump_mmt_buf_as_words(struct mmt_buf *buf);
void dump_mmt_buf_as_words_horiz(struct mmt_buf *buf, const char *pfx);
void dump_mmt_buf_as_word_pairs(struct mmt_buf *buf, const char *(*fun)(uint32_t));
void dump_mmt_buf_as_words_desc(struct mmt_buf *buf, const char *(*fun)(uint32_t));
void dump_args(struct mmt_memory_dump *args, int argc, uint64_t except_ptr);

void describe_nvrm_object(uint32_t cid, uint32_t handle, const char *field_name);

int _field_enabled(const char *name);

#define field_enabled(strct, field) (strct->field || _field_enabled(#field))

extern const char *nvrm_sep, *nvrm_pfx;

static inline void nvrm_reset_pfx()
{
	nvrm_pfx = "";
}

#define print_u64(strct, field)				do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": 0x%016lx", nvrm_pfx, strct->field); nvrm_pfx = nvrm_sep; } while (0)
#define print_u32(strct, field)				do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": 0x%08x",   nvrm_pfx, strct->field); nvrm_pfx = nvrm_sep; } while (0)
#define print_u16(strct, field)				do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": 0x%04x",   nvrm_pfx, strct->field); nvrm_pfx = nvrm_sep; } while (0)

#define print_i32(strct, field)				do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": %d", nvrm_pfx, strct->field); nvrm_pfx = nvrm_sep; } while (0)
#define print_i32_align(strct, field, algn)	do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": %" #algn "d", nvrm_pfx, strct->field); nvrm_pfx = nvrm_sep; } while (0)

#define print_pad_u32(strct, field)			do { if (strct->field) mmt_log_cont("%s%s" #field ": 0x%08x%s",   nvrm_pfx, colors->err, strct->field, colors->reset); nvrm_pfx = nvrm_sep; } while (0)

#define print_str(strct, field)				do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": \"%s\"",   nvrm_pfx, strct->field); nvrm_pfx = nvrm_sep; } while (0)

#define print_ptr(strct, field, args, argc) \
	({ \
		struct mmt_buf *__ret = NULL; \
		if (field_enabled(strct, field)) { \
			if (strct->field) { \
				mmt_log_cont("%s" #field ": 0x%016lx", nvrm_pfx, strct->field); \
				if (argc > 0) \
					__ret = find_ptr(strct->field, args, argc); \
				if (__ret == NULL) \
					mmt_log_cont("%s", " [no data]"); \
			} else \
				mmt_log_cont("%s" #field ": %s", nvrm_pfx, "NULL"); \
		} \
		nvrm_pfx = nvrm_sep; \
		__ret; \
	})

#define print_handle(strct, field, cid) \
	do { \
		if (field_enabled(strct, field)) { \
			print_u32(strct, field); \
			describe_nvrm_object(strct->cid, strct->field, #field); \
		} \
	} while (0)

#define print_cid(strct, field)		print_u32(strct, field)

#define print_class(strct, field) \
	do { \
		mmt_log_cont("%sclass: 0x%04x", nvrm_pfx, strct->field); \
		const char *__n = nvrm_get_class_name(strct->field); \
		if (__n) \
			mmt_log_cont(" [%s]", __n); \
		nvrm_pfx = nvrm_sep; \
	} while (0)

#define print_status(strct, field)			do { if (field_enabled(strct, field)) mmt_log_cont("%s" #field ": %s", nvrm_pfx, nvrm_status(strct->field)); nvrm_pfx = nvrm_sep; } while (0)
#define print_ln() mmt_log_cont_nl()

#endif
