/*
 * Copyright (C) 2013 Marcin Kościelnicki <koriakin@0x04.net>
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NVRM_IOCTL_H
#define NVRM_IOCTL_H

#include <sys/ioctl.h>
#include <inttypes.h>

#define NVRM_IOCTL_MAGIC 'F'

/* used on dev fd */
struct nvrm_ioctl_create_vspace {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t cls;
	uint32_t flags;
	uint32_t unk14; /* maybe pad */
	uint64_t foffset;
	uint64_t limit;
	uint32_t status;
	uint32_t _pad2;
};
#define NVRM_IOCTL_CREATE_VSPACE _IOWR(NVRM_IOCTL_MAGIC, 0x27, struct nvrm_ioctl_create_vspace)

struct nvrm_ioctl_create_simple {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t cls;
	uint32_t status;
};
#define NVRM_IOCTL_CREATE_SIMPLE _IOWR(NVRM_IOCTL_MAGIC, 0x28, struct nvrm_ioctl_create_simple)

struct nvrm_ioctl_destroy {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t status;
};
#define NVRM_IOCTL_DESTROY _IOWR(NVRM_IOCTL_MAGIC, 0x29, struct nvrm_ioctl_destroy)

struct nvrm_ioctl_call {
	uint32_t cid;
	uint32_t handle;
	uint32_t mthd;
	uint32_t _pad;
	uint64_t ptr;
	uint32_t size;
	uint32_t status;
};
#define NVRM_IOCTL_CALL _IOWR(NVRM_IOCTL_MAGIC, 0x2a, struct nvrm_ioctl_call)

struct nvrm_ioctl_create {
	uint32_t cid;
	uint32_t parent;
	uint32_t handle;
	uint32_t cls;
	uint64_t ptr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_CREATE _IOWR(NVRM_IOCTL_MAGIC, 0x2b, struct nvrm_ioctl_create)

/* used on dev fd */
struct nvrm_ioctl_get_param {
	uint32_t cid;
	uint32_t handle; /* device */
	uint32_t key;
	uint32_t value; /* out */
	uint32_t status;
};
#define NVRM_IOCTL_GET_PARAM _IOWR(NVRM_IOCTL_MAGIC, 0x32, struct nvrm_ioctl_get_param)

/* used on dev fd */
struct nvrm_ioctl_query {
	uint32_t cid;
	uint32_t handle;
	uint32_t query;
	uint32_t size;
	uint64_t ptr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_QUERY _IOWR(NVRM_IOCTL_MAGIC, 0x37, struct nvrm_ioctl_query)

struct nvrm_ioctl_memory {
	uint32_t cid;
	uint32_t parent;
	uint32_t cls;
	uint32_t unk0c;
	uint32_t status;
	uint32_t unk14;
	uint64_t vram_total;
	uint64_t vram_free;
	uint32_t vspace;
	uint32_t handle;
	uint32_t unk30;
	uint32_t flags1;
#define NVRM_IOCTL_MEMORY_FLAGS1_USER_HANDLE	0x00004000 /* otherwise 0xcaf..... allocated */
	uint64_t unk38;
	uint32_t flags2;
	uint32_t unk44;
	uint64_t unk48;
	uint32_t flags3;
	uint32_t unk54;
	uint64_t unk58;
	uint64_t size;
	uint64_t align;
	uint64_t base;
	uint64_t limit;
	uint64_t unk80;
	uint64_t unk88;
	uint64_t unk90;
	uint64_t unk98;
};
#define NVRM_IOCTL_MEMORY _IOWR(NVRM_IOCTL_MAGIC, 0x4a, struct nvrm_ioctl_memory)

struct nvrm_ioctl_unk4d {
	uint32_t cid;
	uint32_t handle;
	uint64_t unk08;
	uint64_t unk10;
	uint64_t slen;
	uint64_t sptr;
	uint64_t unk28;
	uint64_t unk30;
	uint64_t unk38; /* out */
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_UNK4D _IOWR(NVRM_IOCTL_MAGIC, 0x4d, struct nvrm_ioctl_unk4d)

struct nvrm_ioctl_host_map {
	uint32_t cid;
	uint32_t subdev;
	uint32_t handle;
	uint32_t _pad;
	uint64_t base;
	uint64_t limit;
	uint64_t foffset;
	uint32_t status;
	uint32_t _pad2;
};
#define NVRM_IOCTL_HOST_MAP _IOWR(NVRM_IOCTL_MAGIC, 0x4e, struct nvrm_ioctl_host_map)

struct nvrm_ioctl_host_unmap {
	uint32_t cid;
	uint32_t subdev;
	uint32_t handle;
	uint32_t _pad;
	uint64_t foffset;
	uint32_t status;
	uint32_t _pad2;
};
#define NVRM_IOCTL_HOST_UNMAP _IOWR(NVRM_IOCTL_MAGIC, 0x4f, struct nvrm_ioctl_host_unmap)

struct nvrm_ioctl_create_dma {
	uint32_t cid;
	uint32_t handle;
	uint32_t cls;
	uint32_t flags;
	uint32_t unk10;
	uint32_t parent;
	uint64_t base;
	uint64_t limit;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_CREATE_DMA _IOWR(NVRM_IOCTL_MAGIC, 0x54, struct nvrm_ioctl_create_dma)

struct nvrm_ioctl_vspace_map {
	uint32_t cid;
	uint32_t dev;
	uint32_t vspace;
	uint32_t handle;
	uint64_t base;
	uint64_t size;
	uint32_t flags;
	uint32_t unk24;
	uint64_t addr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_VSPACE_MAP _IOWR(NVRM_IOCTL_MAGIC, 0x57, struct nvrm_ioctl_vspace_map)

struct nvrm_ioctl_vspace_unmap {
	uint32_t cid;
	uint32_t dev;
	uint32_t vspace;
	uint32_t handle;
	uint64_t unk10;
	uint64_t addr;
	uint32_t status;
	uint32_t _pad;
};
#define NVRM_IOCTL_VSPACE_UNMAP _IOWR(NVRM_IOCTL_MAGIC, 0x58, struct nvrm_ioctl_vspace_unmap)

struct nvrm_ioctl_bind {
	uint32_t cid;
	uint32_t target;
	uint32_t handle;
	uint32_t status;
};
#define NVRM_IOCTL_BIND _IOWR(NVRM_IOCTL_MAGIC, 0x59, struct nvrm_ioctl_bind)

struct nvrm_ioctl_unk5e {
	uint32_t cid;
	uint32_t subdev;
	uint32_t handle;
	uint32_t _pad;
	uint64_t foffset;
	uint64_t ptr; /* to just-mmapped thing */
	uint32_t unk20;
	uint32_t unk24;
};
#define NVRM_IOCTL_UNK5E _IOWR(NVRM_IOCTL_MAGIC, 0x5e, struct nvrm_ioctl_unk5e)

#define NVRM_IOCTL_ESC_BASE 200

struct nvrm_ioctl_card_info {
	struct {
		uint32_t flags;
		uint32_t domain;
		uint8_t bus;
		uint8_t slot;
		uint16_t vendor_id;
		uint16_t device_id;
		uint16_t _pad;
		uint32_t gpu_id;
		uint16_t interrupt;
		uint16_t _pad2;
		uint64_t reg_address;
		uint64_t reg_size;
		uint64_t fb_address;
		uint64_t fb_size;
	} card[32];
};
struct nvrm_ioctl_card_info2 {
	struct {
		uint32_t flags;
		uint32_t domain;
		uint8_t bus;
		uint8_t slot;
		uint8_t function;
		uint8_t _pad0;
		uint16_t vendor_id;
		uint16_t device_id;
		uint32_t _pad1;
		uint32_t gpu_id;
		uint32_t interrupt;
		uint32_t _pad2;
		uint64_t reg_address;
		uint64_t reg_size;
		uint64_t fb_address;
		uint64_t fb_size;
		uint32_t index;
		uint32_t _pad3;
	} card[32];
};
#define NVRM_IOCTL_CARD_INFO _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+0, struct nvrm_ioctl_card_info)
#define NVRM_IOCTL_CARD_INFO2 _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+0, struct nvrm_ioctl_card_info2)

struct nvrm_ioctl_env_info {
	uint32_t pat_supported;
};
#define NVRM_IOCTL_ENV_INFO _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+2, struct nvrm_ioctl_env_info)

struct nvrm_ioctl_create_os_event {
	uint32_t cid;
	uint32_t handle;
	uint32_t ehandle;
	uint32_t fd;
	uint32_t status;
};
#define NVRM_IOCTL_CREATE_OS_EVENT _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+6, struct nvrm_ioctl_create_os_event)

struct nvrm_ioctl_destroy_os_event {
	uint32_t cid;
	uint32_t handle;
	uint32_t fd;
	uint32_t status;
};
#define NVRM_IOCTL_DESTROY_OS_EVENT _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+7, struct nvrm_ioctl_destroy_os_event)

struct nvrm_ioctl_check_version_str {
	uint32_t cmd;
	uint32_t reply;
	char vernum[0x40];
};
#define NVRM_IOCTL_CHECK_VERSION_STR _IOWR(NVRM_IOCTL_MAGIC, NVRM_IOCTL_ESC_BASE+10, struct nvrm_ioctl_check_version_str)


#define NVRM_STATUS_SUCCESS		0
#define NVRM_STATUS_ALREADY_EXISTS_SUB	5	/* like 6, but for subdevice-relative stuff */
#define NVRM_STATUS_ALREADY_EXISTS	6	/* tried to create object for eg. device that already has one */
#define NVRM_STATUS_INVALID_PARAM	8	/* NULL param to create, ... */
#define NVRM_STATUS_INVALID_DEVICE	11	/* NVRM_CLASS_DEVICE devid out of range */
#define NVRM_STATUS_INVALID_MTHD	12	/* invalid mthd to call */
#define NVRM_STATUS_OBJECT_ERROR	26	/* object model violation - wrong parent class, tried to create object with existing handle, nonexistent object, etc. */
#define NVRM_STATUS_NO_HW		29	/* not supported on this card */
#define NVRM_STATUS_MTHD_SIZE_MISMATCH	32	/* invalid param size for a mthd */
#define NVRM_STATUS_ADDRESS_FAULT	34	/* basically -EFAULT */
#define NVRM_STATUS_MTHD_CLASS_MISMATCH	41	/* invalid mthd for given class */

#endif
