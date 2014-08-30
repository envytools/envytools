#ifndef DEMMT_NVRM_H
#define DEMMT_NVRM_H

#include "mmt_bin_decode_nvidia.h"

int demmt_nv_ioctl_pre(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state);
int demmt_nv_ioctl_post(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state);

void demmt_ioctl_post(struct mmt_ioctl_post *ctl, void *state);
void demmt_nv_memory_dump(struct mmt_nvidia_memory_dump *d, void *state);
void demmt_nv_memory_dump_cont(struct mmt_buf *b, void *state);
void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state);
void demmt_nv_create_driver_object(struct mmt_nvidia_create_driver_object *create, void *state);
void demmt_nv_create_device_object(struct mmt_nvidia_create_device_object *create, void *state);
void demmt_nv_create_context_object(struct mmt_nvidia_create_context_object *create, void *state);
void demmt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state);
void demmt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state);
void demmt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state);

#endif
