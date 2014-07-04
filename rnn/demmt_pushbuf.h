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

struct ib_decode_state
{
	int word;

	uint64_t address;
	int unk8;
	int not_main;
	int size;
	int no_prefetch;

	struct pushbuf_decode_state pstate;
	struct buffer *last_buffer;
};

void pushbuf_add_object(uint32_t handle, uint32_t class);

void pushbuf_decode_start(struct pushbuf_decode_state *state);
void pushbuf_decode(struct pushbuf_decode_state *state, uint32_t data, char *output);
void pushbuf_decode_end(struct pushbuf_decode_state *state);

void ib_decode_start(struct ib_decode_state *state);
void ib_decode(struct ib_decode_state *state, uint32_t data, char *output);
void ib_decode_end(struct ib_decode_state *state);

#endif
