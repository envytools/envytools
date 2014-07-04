#ifndef DEMMT_PUSHBUF_H
#define DEMMT_PUSHBUF_H

#include <stdint.h>

struct pushbuf_decode_state
{
	int incr;
	int subchan;
	int addr;
	int size;
	int skip;
	int pushbuf_invalid;
	int next_command_offset;
};

void pushbuf_decode_start(struct pushbuf_decode_state *state);
void pushbuf_decode(struct pushbuf_decode_state *state, uint32_t data, char *output);
void pushbuf_decode_end(struct pushbuf_decode_state *state);
void pushbuf_add_object(uint32_t handle, uint32_t class);

#endif
