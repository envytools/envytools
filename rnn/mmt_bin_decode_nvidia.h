#ifndef MMT_BIN_DECODE_NVIDIA_H
#define MMT_BIN_DECODE_NVIDIA_H

#include "mmt_bin_decode.h"

struct mmt_message_nv
{
	struct mmt_message msg_type;
	uint8_t subtype;
} __packed;

struct mmt_nvidia_create_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint32_t class;
	struct mmt_buf name;
} __packed;

struct mmt_nvidia_destroy_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
} __packed;

struct mmt_ioctl_pre
{
	struct mmt_message_nv msg_type;
	uint32_t fd;
	uint32_t id;
	struct mmt_buf data;
} __packed;

struct mmt_ioctl_post
{
	struct mmt_message_nv msg_type;
	uint32_t fd;
	uint32_t id;
	struct mmt_buf data;
} __packed;

struct mmt_memory_dump_prefix
{
	struct mmt_message_nv msg_type;
	uint64_t addr;
	struct mmt_buf str;
} __packed;

struct mmt_nvidia_call_method
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
} __packed;

struct mmt_nvidia_create_mapped_object
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t type;
	uint64_t mmap_offset;
} __packed;

struct mmt_nvidia_create_dma_object
{
	struct mmt_message_nv msg_type;
	uint32_t name;
	uint32_t type;
	uint32_t parent;
} __packed;

struct mmt_nvidia_alloc_map
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint64_t mmap_offset;
} __packed;

struct mmt_nvidia_gpu_map
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint32_t gpu_start;
	uint32_t len;
} __packed;

struct mmt_nvidia_gpu_map2
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint64_t gpu_start;
	uint32_t len;
} __packed;

struct mmt_nvidia_gpu_unmap
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint32_t gpu_start;
} __packed;

struct mmt_nvidia_gpu_unmap2
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint64_t gpu_start;
} __packed;

struct mmt_nvidia_bind
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
} __packed;

struct mmt_nvidia_mmap
{
	struct mmt_message_nv msg_type;
	uint64_t offset;
	uint32_t id;
	uint64_t start;
	uint64_t len;
	uint64_t data1;
	uint64_t data2;
} __packed;

struct mmt_nvidia_mmap2
{
	struct mmt_message_nv msg_type;
	uint64_t offset;
	uint32_t prot;
	uint32_t flags;
	uint32_t fd;
	uint32_t id;
	uint64_t start;
	uint64_t len;
	uint64_t data1;
	uint64_t data2;
} __packed;

struct mmt_nvidia_unmap
{
	struct mmt_message_nv msg_type;
	uint32_t data1;
	uint32_t data2;
	uint64_t mmap_offset;
} __packed;

struct mmt_nvidia_create_driver_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
	uint32_t obj2;
	uint64_t addr;
} __packed;

struct mmt_nvidia_create_device_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
} __packed;

struct mmt_nvidia_create_context_object
{
	struct mmt_message_nv msg_type;
	uint32_t obj1;
} __packed;

struct mmt_nvidia_call_method_data
{
	struct mmt_message_nv msg_type;
	uint32_t cnt;
	uint64_t tx;
	struct mmt_buf data;
} __packed;

struct mmt_nvidia_ioctl_4d
{
	struct mmt_message_nv msg_type;
	struct mmt_buf str;
} __packed;

struct mmt_nvidia_mmiotrace_mark
{
	struct mmt_message_nv msg_type;
	struct mmt_buf str;
} __packed;

struct mmt_nouveau_pushbuf_data
{
	struct mmt_message_nv msg_type;
	struct mmt_buf data;
} __packed;

struct mmt_memory_dump
{
	uint64_t addr;
	struct mmt_buf *str;
	struct mmt_buf *data;
};

struct mmt_nvidia_decode_funcs
{
	const struct mmt_decode_funcs base;
	void (*create_object)(struct mmt_nvidia_create_object *create, void *state);
	void (*destroy_object)(struct mmt_nvidia_destroy_object *destroy, void *state);
	void (*ioctl_pre)(struct mmt_ioctl_pre *ctl, void *state, struct mmt_memory_dump *args, int argc);
	void (*ioctl_post)(struct mmt_ioctl_post *ctl, void *state, struct mmt_memory_dump *args, int argc);
	void (*memory_dump)(struct mmt_memory_dump_prefix *d, struct mmt_buf *b, void *state);
	void (*call_method)(struct mmt_nvidia_call_method *m, void *state);
	void (*create_mapped)(struct mmt_nvidia_create_mapped_object *p, void *state);
	void (*create_dma_object)(struct mmt_nvidia_create_dma_object *create, void *state);
	void (*alloc_map)(struct mmt_nvidia_alloc_map *alloc, void *state);
	void (*gpu_map)(struct mmt_nvidia_gpu_map *map, void *state);
	void (*gpu_map2)(struct mmt_nvidia_gpu_map2 *map, void *state);
	void (*gpu_unmap)(struct mmt_nvidia_gpu_unmap *unmap, void *state);
	void (*gpu_unmap2)(struct mmt_nvidia_gpu_unmap2 *unmap, void *state);
	void (*mmap)(struct mmt_nvidia_mmap *map, void *state);
	void (*mmap2)(struct mmt_nvidia_mmap2 *map, void *state);
	void (*unmap)(struct mmt_nvidia_unmap *unmap, void *state);
	void (*bind)(struct mmt_nvidia_bind *bnd, void *state);
	void (*create_driver_object)(struct mmt_nvidia_create_driver_object *create, void *state);
	void (*create_device_object)(struct mmt_nvidia_create_device_object *create, void *state);
	void (*create_context_object)(struct mmt_nvidia_create_context_object *create, void *state);
	void (*call_method_data)(struct mmt_nvidia_call_method_data *call, void *state);
	void (*ioctl_4d)(struct mmt_nvidia_ioctl_4d *ctl, void *state);
	void (*mmiotrace_mark)(struct mmt_nvidia_mmiotrace_mark *mark, void *state);
	void (*nouveau_gem_pushbuf_data)(struct mmt_nouveau_pushbuf_data *data, void *state);
};

void mmt_decode_nvidia(struct mmt_nvidia_decode_funcs *funcs, void *state);

#endif
