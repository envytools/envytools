#ifndef DEMMT_MACRO_H
#define DEMMT_MACRO_H

#include <stdint.h>
#include "pushbuf.h"

struct buffer;

struct macro_interpreter_state
{
	uint32_t *code;
	uint32_t words;

	uint32_t pc;
	uint32_t regs[8];
	uint32_t mthd;
	uint32_t incr;

	int aborted;

	const uint32_t *macro_param;

	uint32_t delayed_pc;
	uint32_t exit_when_0;

	uint32_t lastpc;
	uint32_t backward_jumps;

	struct obj *obj;
};

struct macro_state
{
	struct buffer *buffer;
	uint32_t last_code_pos;
	uint32_t cur_code_pos;
	uint32_t last_entry_pos;
	struct
	{
		uint32_t start;
		uint32_t words;
	}
	entries[0x80];

	struct macro_interpreter_state istate;
};

int decode_macro(struct pushbuf_decode_state *pstate, struct macro_state *macro);

extern int macro_rt_verbose;
extern int macro_rt;
extern int macro_dis_enabled;
#endif
