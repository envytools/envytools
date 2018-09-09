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
		struct mmt_buf *buf, uint64_t ret, uint64_t err, void *state,
		struct mmt_memory_dump *args, int argc);
void decode_nvrm_ioctl_query(struct nvrm_ioctl_query *s, struct mmt_memory_dump *args, int argc);
void decode_nvrm_ioctl_call(struct nvrm_ioctl_call *s, struct mmt_memory_dump *args, int argc);

const char *nvrm_status(uint32_t status);
const char *nvrm_get_class_name(uint32_t cls);

void describe_nvrm_object(uint32_t cid, uint32_t handle, const char *field_name);

int _nvrm_field_enabled(const char *name);

#define nvrm_field_enabled(strct, field) ((strct)->field || _nvrm_field_enabled(#field))

extern const char *nvrm_sep, *nvrm_pfx;

static inline void nvrm_reset_pfx()
{
	nvrm_pfx = "";
}

#define nvrm_print_x64(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_x64(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_x32(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_x32(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_x16(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_x16(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_x8(strct, field)					do { if (nvrm_field_enabled(strct, field)) _print_x8 (nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)

#define nvrm_print_d64_align(strct, field, algn)	do { if (nvrm_field_enabled(strct, field)) _print_d64_align(nvrm_pfx, strct, field, #algn); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_d32_align(strct, field, algn)	do { if (nvrm_field_enabled(strct, field)) _print_d32_align(nvrm_pfx, strct, field, #algn); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_d16_align(strct, field, algn)	do { if (nvrm_field_enabled(strct, field)) _print_d16_align(nvrm_pfx, strct, field, #algn); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_d8_align(strct, field, algn)		do { if (nvrm_field_enabled(strct, field)) _print_d8_align (nvrm_pfx, strct, field, #algn); nvrm_pfx = nvrm_sep; } while (0)

#define nvrm_print_d64(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_d64(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_d32(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_d32(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_d16(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_d16(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_d8(strct, field)					do { if (nvrm_field_enabled(strct, field)) _print_d8 (nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)

#define nvrm_print_pad_x32(strct, field)			do { if ((strct)->field) mmt_log_cont("%s%s" #field ": 0x%08x%s",   nvrm_pfx, colors->err, (strct)->field, colors->reset); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_pad_x8(strct, field)				do { if ((strct)->field) mmt_log_cont("%s%s" #field ": 0x%02x%s",   nvrm_pfx, colors->err, (strct)->field, colors->reset); nvrm_pfx = nvrm_sep; } while (0)

#define nvrm_print_str(strct, field)				do { if (nvrm_field_enabled(strct, field)) _print_str(nvrm_pfx, strct, field); nvrm_pfx = nvrm_sep; } while (0)

#define nvrm_print_ptr(strct, field, args, argc) \
	({ \
		struct mmt_buf *__ret = NULL; \
		if (nvrm_field_enabled(strct, field)) { \
			if (strct->field) { \
				mmt_log_cont("%s" #field ": 0x%016" PRIx64 "", nvrm_pfx, strct->field); \
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

#define nvrm_print_handle(strct, field, cid) \
	do { \
		if (nvrm_field_enabled(strct, field)) { \
			nvrm_print_x32(strct, field); \
			describe_nvrm_object(strct->cid, strct->field, #field); \
		} \
	} while (0)

#define nvrm_print_cid(strct, field)		nvrm_print_x32(strct, field)

#define nvrm_print_class(strct, field) \
	do { \
		mmt_log_cont("%sclass: 0x%04x", nvrm_pfx, strct->field); \
		const char *__n = nvrm_get_class_name(strct->field); \
		if (__n) \
			mmt_log_cont(" [%s]", __n); \
		nvrm_pfx = nvrm_sep; \
	} while (0)

#define nvrm_print_status(strct, field)			do { if (nvrm_field_enabled(strct, field)) mmt_log_cont("%s" #field ": %s", nvrm_pfx, nvrm_status(strct->field)); nvrm_pfx = nvrm_sep; } while (0)
#define nvrm_print_ln() mmt_log_cont_nl()

#endif
