#ifndef DEMMT_NVRM_H
#define DEMMT_NVRM_H

#include "mmt_bin_decode_nvidia.h"

int demmt_nv_ioctl_pre(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc);
int demmt_nv_ioctl_post(uint32_t fd, uint32_t id, uint8_t dir, uint8_t nr, uint16_t size,
		struct mmt_buf *buf, void *state, struct mmt_memory_dump *args, int argc);

void demmt_memory_dump(struct mmt_memory_dump_prefix *d, struct mmt_buf *b, void *state);
void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state);
void demmt_nv_mmap2(struct mmt_nvidia_mmap2 *mm, void *state);
void demmt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state);
void demmt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state);
void demmt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state);
const char *demmt_nvrm_get_class_name(uint32_t cls);

#endif
