#ifndef DEMMT_BUFFER_H
#define DEMMT_BUFFER_H

#include <stdint.h>
#include "demmt.h"
#include "mmt_bin_decode.h"
#include "pushbuf.h"
#include "region.h"

struct gpu_object;

struct cpu_mapping
{
	uint32_t id;
	int fd;
	uint32_t subdev;

	uint64_t mmap_offset;
	uint64_t cpu_addr;
	uint64_t object_offset;
	uint64_t length;
	uint8_t *data;
	struct regions written_regions;

	struct cpu_mapping *next; // in gpu_object

	struct gpu_object *object;

	struct
	{
		int is;
		uint32_t offset;
		struct ib_decode_state state;
	}
	ib;

	struct
	{
		int is;
		struct user_decode_state state;
	}
	user;
};

struct gpu_mapping
{
	int fd;
	uint32_t dev;
	uint32_t vspace;

	uint64_t object_offset;
	uint64_t address;
	uint64_t length;
	//uint8_t *data;
	struct gpu_object *object;

	struct gpu_mapping *next;
};

struct gpu_object
{
	int fd;
	uint32_t cid;
	uint32_t handle;
	uint32_t parent;
	struct gpu_object *parent_object;

	struct gpu_object **children_objects;
	int children_space;

	uint32_t class_;

	uint64_t length;
	uint8_t *data;
	struct regions written_regions;

	struct cpu_mapping *cpu_mappings;
	struct gpu_mapping *gpu_mappings;

	struct gpu_object *next;

	struct
	{
		char *desc;
		uint64_t address;
	}
	usage[MAX_USAGES];

	void *class_data;
	void (*class_data_destroy)(struct gpu_object *gpu_obj);
};

extern struct gpu_object *gpu_objects;
extern struct cpu_mapping *cpu_mappings[MAX_ID];

void buffer_ioctl_pre(int print_raw);
void buffer_mmap(uint32_t id, uint32_t fd, uint64_t cpu_start, uint64_t len, uint64_t mmap_offset);
void buffer_munmap(uint32_t id);
void buffer_mremap(struct mmt_mremap *mm);
void buffer_register_mmt_write(struct mmt_write *w);
void buffer_register_mmt_read(struct mmt_read *r);
void buffer_flush();

struct gpu_object *gpu_object_add(uint32_t fd, uint32_t cid, uint32_t parent, uint32_t handle, uint32_t class_);
struct gpu_object *gpu_object_find(uint32_t cid, uint32_t handle);
void gpu_object_destroy(struct gpu_object *obj);

struct gpu_mapping *gpu_mapping_find(uint64_t address, struct gpu_object *dev);
void *gpu_mapping_get_data(struct gpu_mapping *mapping, uint64_t address, uint64_t length);
void gpu_mapping_destroy(struct gpu_mapping *gpu_mapping);

struct cpu_mapping *gpu_addr_to_cpu_mapping(struct gpu_mapping *gpu_mapping, uint64_t gpu_address);
uint64_t cpu_mapping_to_gpu_addr(struct cpu_mapping *mapping, uint64_t offset);
void disconnect_cpu_mapping_from_gpu_object(struct cpu_mapping *cpu_mapping);

void gpu_mapping_register_write(struct gpu_mapping *mapping, uint64_t address, uint32_t len, const void *data);
void gpu_mapping_register_copy(struct gpu_mapping *dst_mapping, uint64_t dst_address,
		struct gpu_mapping *src_mapping, uint64_t src_address, uint32_t len);

#endif
