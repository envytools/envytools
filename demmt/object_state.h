#ifndef DEMMT_OBJECTS_H
#define DEMMT_OBJECTS_H

#include <stdint.h>
#include "pushbuf.h"

struct gpu_object_decoder
{
	uint32_t class_;
	// do not use newlines or any logging macros (mmt_debug, mmt_log, etc); always called first
	void (*decode_terse)(struct pushbuf_decode_state *);
	// do whatever you like to do
	void (*decode_verbose)(struct pushbuf_decode_state *);

	// internal
	int disabled;
};
extern struct gpu_object_decoder obj_decoders[];

const struct gpu_object_decoder *demmt_get_decoder(uint32_t class_);

#endif
