#ifndef BUFFER_DECODE_H
#define BUFFER_DECODE_H

#include "buffer.h"

void buffer_decode_register_write(struct cpu_mapping *mapping, uint32_t start, uint32_t len);

#endif
