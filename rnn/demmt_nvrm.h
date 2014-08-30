#ifndef DEMMT_NVRM_H
#define DEMMT_NVRM_H

#include "mmt_bin_decode_nvidia.h"

int demmt_nv_ioctl_pre(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state);
int demmt_nv_ioctl_post(uint32_t id, uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *buf, void *state);

void demmt_nv_create_object(struct mmt_nvidia_create_object *create, void *state);
void demmt_nv_destroy_object(struct mmt_nvidia_destroy_object *destroy, void *state);
void demmt_ioctl_post(struct mmt_ioctl_post *ctl, void *state);
void demmt_nv_memory_dump(struct mmt_nvidia_memory_dump *d, void *state);
void demmt_nv_memory_dump_cont(struct mmt_buf *b, void *state);
void demmt_nv_call_method(struct mmt_nvidia_call_method *m, void *state);
void demmt_nv_create_mapped(struct mmt_nvidia_create_mapped_object *p, void *state);
void demmt_nv_create_dma_object(struct mmt_nvidia_create_dma_object *create, void *state);
void demmt_nv_alloc_map(struct mmt_nvidia_alloc_map *alloc, void *state);
void demmt_nv_gpu_map1(struct mmt_nvidia_gpu_map *map, void *state);
void demmt_nv_gpu_map2(struct mmt_nvidia_gpu_map2 *map, void *state);
void demmt_nv_gpu_unmap1(struct mmt_nvidia_gpu_unmap *unmap, void *state);
void demmt_nv_gpu_unmap2(struct mmt_nvidia_gpu_unmap2 *unmap, void *state);
void demmt_nv_mmap(struct mmt_nvidia_mmap *mm, void *state);
void demmt_nv_unmap(struct mmt_nvidia_unmap *mm, void *state);
void demmt_nv_bind(struct mmt_nvidia_bind *bnd, void *state);
void demmt_nv_create_driver_object(struct mmt_nvidia_create_driver_object *create, void *state);
void demmt_nv_create_device_object(struct mmt_nvidia_create_device_object *create, void *state);
void demmt_nv_create_context_object(struct mmt_nvidia_create_context_object *create, void *state);
void demmt_nv_call_method_data(struct mmt_nvidia_call_method_data *call, void *state);
void demmt_nv_ioctl_4d(struct mmt_nvidia_ioctl_4d *ctl, void *state);
void demmt_nv_mmiotrace_mark(struct mmt_nvidia_mmiotrace_mark *mark, void *state);

#endif
