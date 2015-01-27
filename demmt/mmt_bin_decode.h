#ifndef MMT_BIN_DECODE_H
#define MMT_BIN_DECODE_H

#include <stdint.h>

#define MMT_BUF_SIZE 64 * 1024
extern unsigned char mmt_buf[MMT_BUF_SIZE];
extern unsigned int mmt_idx;

void mmt_check_eor(unsigned int sz);
void *mmt_load_data(unsigned int sz);
void *mmt_load_data_with_prefix(unsigned int sz, unsigned int pfx, int eof_allowed);
void *mmt_load_initial_data();
void mmt_dump_next();

#define __packed  __attribute__((__packed__))

struct mmt_message
{
	uint8_t type;
} __packed;

struct mmt_buf
{
	uint32_t len;
	uint8_t data[0];
} __packed;

void mmt_buf_check_sanity(struct mmt_buf *buf);

struct mmt_memory_dump
{
	uint64_t addr;
	struct mmt_buf *str;
	struct mmt_buf *data;
};

struct mmt_buf *find_ptr(uint64_t ptr, struct mmt_memory_dump *args, int argc);

struct mmt_write
{
	struct mmt_message msg_type;
	uint32_t id;
	uint32_t offset;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct mmt_write2
{
	struct mmt_message msg_type;
	uint64_t addr;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct mmt_read
{
	struct mmt_message msg_type;
	uint32_t id;
	uint32_t offset;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct mmt_read2
{
	struct mmt_message msg_type;
	uint64_t addr;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct mmt_mmap
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t start;
	uint64_t len;
} __packed;

struct mmt_mmap2
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t prot;
	uint32_t flags;
	uint32_t fd;
	uint32_t id;
	uint64_t start;
	uint64_t len;
} __packed;

struct mmt_unmap
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t start;
	uint64_t len;
	uint64_t data1;
	uint64_t data2;
} __packed;

struct mmt_open
{
	struct mmt_message msg_type;
	uint32_t flags;
	uint32_t mode;
	uint32_t ret;
	struct mmt_buf path;
} __packed;

struct mmt_mremap
{
	struct mmt_message msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t old_start;
	uint64_t old_len;
	uint64_t data1;
	uint64_t data2;
	uint64_t start;
	uint64_t len;
} __packed;

struct mmt_write_syscall
{
	struct mmt_message msg_type;
	uint32_t fd;
	struct mmt_buf data;
} __packed;

struct mmt_dup_syscall
{
	struct mmt_message msg_type;
	uint32_t oldfd;
	uint32_t newfd;
} __packed;

struct mmt_sync
{
	struct mmt_message msg_type;
	uint32_t id;
} __packed;

struct mmt_ioctl_pre_v2
{
	struct mmt_message msg_type;
	uint32_t fd;
	uint32_t id;
	struct mmt_buf data;
} __packed;

struct mmt_ioctl_post_v2
{
	struct mmt_message msg_type;
	uint32_t fd;
	uint32_t id;
	uint64_t ret;
	uint64_t err;
	struct mmt_buf data;
} __packed;

struct mmt_memory_dump_v2_prefix
{
	struct mmt_message msg_type;
	uint64_t addr;
} __packed;

struct mmt_decode_funcs
{
	void (*memread)(struct mmt_read *w, void *state);
	void (*memwrite)(struct mmt_write *w, void *state);
	void (*mmap)(struct mmt_mmap *m, void *state);
	void (*mmap2)(struct mmt_mmap2 *m, void *state);
	void (*munmap)(struct mmt_unmap *mm, void *state);
	void (*mremap)(struct mmt_mremap *mm, void *state);
	void (*open)(struct mmt_open *o, void *state);
	void (*msg)(uint8_t *data, unsigned int len, void *state);
	void (*write_syscall)(struct mmt_write_syscall *o, void *state);
	void (*dup_syscall)(struct mmt_dup_syscall *o, void *state);
	void (*sync)(struct mmt_sync *s, void *state);
	void (*ioctl_pre)(struct mmt_ioctl_pre_v2 *ctl, void *state, struct mmt_memory_dump *args, int argc);
	void (*ioctl_post)(struct mmt_ioctl_post_v2 *ctl, void *state, struct mmt_memory_dump *args, int argc);
	void (*memread2)(struct mmt_read2 *w, void *state);
	void (*memwrite2)(struct mmt_write2 *w, void *state);
};

void mmt_decode(const struct mmt_decode_funcs *funcs, void *state);

#endif
