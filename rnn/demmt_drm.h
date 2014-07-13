#ifndef DEMMT_DRM_H
#define DEMMT_DRM_H

#include "mmt_bin_decode.h"

#ifdef LIBDRM_AVAILABLE
int demmt_drm_ioctl_pre(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *data, void *state);
int demmt_drm_ioctl_post(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *data, void *state);
#else
static inline int demmt_drm_ioctl_pre(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *data, void *state)
{
	return 1;
}
static inline int demmt_drm_ioctl_post(uint8_t dir, uint8_t nr, uint16_t size, struct mmt_buf *data, void *state)
{
	return 1;
}
#endif

#endif
