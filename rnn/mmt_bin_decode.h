#ifndef MMT_BIN_DECODE_H
#define MMT_BIN_DECODE_H

#include <stdint.h>

#define MMT_BUF_SIZE 64 * 1024
extern unsigned char mmt_buf[MMT_BUF_SIZE];
extern int mmt_idx;

void mmt_check_eor(int sz);
void *mmt_load_data(int sz);
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

struct mmt_write
{
	struct mmt_message msg_type;
	uint32_t id;
	uint32_t offset;
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

struct mmt_mmap
{
	struct mmt_message msg_type;
	uint64_t offset;
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

struct mmt_decode_funcs
{
	void (*memread)(struct mmt_read *w, void *state);
	void (*memwrite)(struct mmt_write *w, void *state);
	void (*mmap)(struct mmt_mmap *m, void *state);
	void (*munmap)(struct mmt_unmap *mm, void *state);
	void (*mremap)(struct mmt_mremap *mm, void *state);
	void (*open)(struct mmt_open *o, void *state);
	void (*msg)(uint8_t *data, int len, void *state);
};

void mmt_decode(const struct mmt_decode_funcs *funcs, void *state);

#endif
