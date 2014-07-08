/*
 * Copyright (C) 2010 Francisco Jerez.
 * Copyright (C) 2014 Marcin Ślusarz <marcin.slusarz@gmail.com>.
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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "rnndec.h"
#include "util.h"
#include "demmt.h"
#include "demmt_pushbuf.h"
#include "demmt_objects.h"

void pushbuf_decode_start(struct pushbuf_decode_state *state)
{
	memset(state, 0, sizeof(*state));
}

struct obj
{
	uint32_t handle;
	uint32_t class;
	char *name;
	struct rnndeccontext *ctx;
};

static struct obj *subchans[8] = { NULL };
#define MAX_OBJECTS 256
static struct obj objects[MAX_OBJECTS];

void pushbuf_add_object(uint32_t handle, uint32_t class)
{
	struct rnnenum *chs = rnn_findenum(rnndb, "chipset");
	struct rnnenum *cls = rnn_findenum(rnndb, "obj-class");
	struct rnnvalue *v;
	struct obj *obj;
	int i;

	if (!cls || !chs)
	{
		fprintf(stderr, "No obj-class/chipset enum found\n");
		abort();
	}

	for (i = 0; obj = &objects[i], i < MAX_OBJECTS; i++)
	{
		if (obj->handle)
			continue;

		obj->handle = handle;
		obj->class = class;
		obj->ctx = rnndec_newcontext(rnndb);
		obj->ctx->colors = colors;

		v = NULL;
		FINDARRAY(chs->vals, v, v->value == chipset);
		rnndec_varadd(obj->ctx, "chipset", v ? v->name : "NV01");

		v = NULL;
		FINDARRAY(cls->vals, v, v->value == class);
		obj->name = v ? v->name : NULL;
		rnndec_varadd(obj->ctx, "obj-class", v ? v->name : "NV01_NULL");

		return;
	}

	fprintf(stderr, "Too many objects\n");
	abort();
}

static struct obj *get_object(uint32_t handle)
{
	struct obj *objs = objects;
	int i;
	if (handle == 0)
		return NULL;

	for (i = 0; i < MAX_OBJECTS; i++)
		if (objs[i].handle == handle)
			return &objs[i];

	if (chipset >= 0xc0)
	{
		pushbuf_add_object(handle, handle & 0xffff);
		return get_object(handle);
	}

	return NULL;
}

static void decode_header(struct pushbuf_decode_state *state, uint32_t data, char *output)
{
	struct obj *obj = subchans[state->subchan];

	if (!state->long_command)
		sprintf(output, "size %d, subchannel %d (object handle 0x%x), offset 0x%04x, %s",
				state->size, state->subchan, (obj ? obj->handle : 0), state->addr,
			    (state->incr ? "increment" : "constant"));
	else
		sprintf(output, "size ?, subchannel %d (object handle 0x%x), offset 0x%04x, %s",
				state->subchan, (obj ? obj->handle : 0), state->addr,
			    (state->incr ? "increment" : "constant"));
}

static void decode_method(struct pushbuf_decode_state *state, uint32_t data, char *output)
{
	struct rnndecaddrinfo *ai;
	struct obj *obj = subchans[state->subchan];
	char *dec_obj = NULL;
	char *dec_addr = NULL;
	char *dec_val = NULL;

	/* get an object name */
	if (obj && obj->name)
		asprintf(&dec_obj, "%s%s%s", colors->rname, obj->name, colors->reset);
	else
		asprintf(&dec_obj, "%sOBJ%X%s", colors->err, obj ? obj->class : 0, colors->reset);

	/* get the method name and value */
	if (obj)
	{
		ai = rnndec_decodeaddr(obj->ctx, domain, state->addr, 1);

		dec_addr = ai->name;
		dec_val = rnndec_decodeval(obj->ctx, ai->typeinfo, data, ai->width);

		free(ai);
	}
	else
		asprintf(&dec_addr, "%s0x%x%s", colors->err, state->addr, colors->reset);

	/* write it */
	if (state->addr == 0)
		sprintf(output, "  %s mapped to subchannel %d", dec_obj, state->subchan);
	else if (dec_val)
		sprintf(output, "  %s.%s = %s", dec_obj, dec_addr, dec_val);
	else
		sprintf(output, "  %s.%s", dec_obj, dec_addr);

	free(dec_val);
	free(dec_addr);
	free(dec_obj);
}

void pushbuf_decode(struct pushbuf_decode_state *state, uint32_t data, char *output, int *addr, int safe)
{
	if (addr)
		*addr = -1;
	if (state->skip)
	{
		strcpy(output, "SKIP");
		state->skip--;
		return;
	}

	if (state->size == 0)
	{
		if (data == 0)
		{
			strcpy(output, "NOP");
			return;
		}

		if (chipset >= 0xc0)
		{
			int mode = (data & 0xe0000000) >> 29;

			state->addr = (data & 0x1fff) << 2;
			state->subchan = (data & 0xe000) >> 13;
			state->size = (data & 0x1fff0000) >> 16;
			if (mode == 3)
				state->incr = 0;
			else if (mode == 5)
				state->incr = 1;
			else if (mode == 1)
				state->incr = state->size;
			else if (mode == 4)
			{
				decode_method(state, state->size, output);
				state->size = 0;
				if (addr)
					*addr = state->addr;
				return;
			}
			else if (mode == 0)
			{
				state->incr = 1;
				state->size = (data & 0x1ffc0000) >> 18;
				int type = (data & 0x30000) >> 16;
				if (type != 0)
				{
					state->size = 0;
					if (type == 1)
						sprintf(output, "SLI cond, mask: 0x%x", (data & 0xfff0) >> 4);
					else if (type == 2)
						sprintf(output, "SLI user mask store: 0x%x", (data & 0xfff0) >> 4);
					else if (type == 3)
						sprintf(output, "SLI cond from user mask");
					return;
				}

				if (!state->pushbuf_invalid)
					mmt_log("unusual, old-style inc mthd%s\n", "");
			}
			else if (mode == 2)
			{
				state->incr = 0;
				state->size = (data & 0x1ffc0000) >> 18;
				int type = (data & 0x30000) >> 16;
				if (type != 0)
				{
					state->size = 0;
					sprintf(output, "invalid old-style non-inc mthd, type: %d", type);
					return;
				}

				if (!state->pushbuf_invalid)
					mmt_log("unusual, old-style non-inc mthd%s\n", "");
			}
			else
			{
				state->size = 0;
				sprintf(output, "unknown mode %d", mode);
				return;
			}
		}
		else
		{
			if (state->long_command)
			{
				state->size = data & 0xffffff;
				state->long_command = 0;
				sprintf(output, "size %d", state->size);
				return;
			}

			int mode = (data & 0xe0000000) >> 29;
			state->addr = data & 0x1ffc;
			state->subchan = (data & 0xe000) >> 13;
			state->size = (data & 0x1ffc0000) >> 18;
			int type = data & 3;

			if (type == 0)
			{
				if (mode == 0)
				{
					type = (data & 0x30000) >> 16;
					if (type == 0)
						state->incr = state->size;
					else if (type == 1)
					{
						state->size = 0;
						sprintf(output, "SLI cond, mask: 0x%x", (data & 0xfff0) >> 4);
						return;
					}
					else if (type == 2)
					{
						state->size = 0;
						sprintf(output, "return");
						return;
					}
					else if (type == 3)
					{
						state->incr = 0;
						state->size = 0;
						state->long_command = 1;
					}
				}
				else if (mode == 1)
				{
					sprintf(output, "jump (old) to 0x%x", data & 0x1ffffffc);
					return;
				}
				else if (mode == 2)
					state->incr = 0;
				else
				{
					sprintf(output, "unknown mode, top 3 bits: %d", mode);
					state->size = 0;
					return;
				}
			}
			else if (type == 1)
			{
				sprintf(output, "jump to 0x%x", data & 0xfffffffc);
				state->size = 0;
				return;
			}
			else if (type == 2)
			{
				sprintf(output, "call 0x%x", data & 0xfffffffc);
				state->size = 0;
				return;
			}
			else
			{
				sprintf(output, "unknown type, bottom 2 bits: %d", type);
				state->size = 0;
				return;
			}
		}

		decode_header(state, data, output);
		if (guess_invalid_pushbuf && subchans[state->subchan] == NULL && state->addr != 0 && state->pushbuf_invalid == 0)
		{
			mmt_log("subchannel %d does not have bound object and first command does not bind it, marking this buffer invalid\n", state->subchan);
			state->pushbuf_invalid = 1;
		}
	}
	else
	{
		if (state->addr == 0)
		{
			if (subchans[state->subchan] == NULL)
			{
				if (guess_invalid_pushbuf && state->pushbuf_invalid)
					mmt_log("this is invalid buffer, not going to bind object 0x%08x to subchannel %d\n", data, state->subchan);
				else
					subchans[state->subchan] = get_object(data);
			}
			else
			{
				if (data != subchans[state->subchan]->handle)
				{
					if (safe)
						subchans[state->subchan] = get_object(data);
					else
					{
						if (guess_invalid_pushbuf && state->pushbuf_invalid == 0)
						{
							mmt_log("subchannel %d is already taken, marking this buffer invalid\n", state->subchan);
							state->pushbuf_invalid = 1;
						}
					}
				}
			}
		}

		decode_method(state, data, output);
		if (addr)
			*addr = state->addr;

		if (state->incr)
		{
			state->addr += 4;
			state->incr--;
		}

		state->size--;
	}
}

void pushbuf_decode_end(struct pushbuf_decode_state *state)
{
}


void ib_decode_start(struct ib_decode_state *state)
{
	memset(state, 0, sizeof(*state));
	pushbuf_decode_start(&state->pstate);
}

static void ib_print(struct ib_decode_state *state)
{
	char cmdoutput[1024];
	uint64_t cur = state->address - state->last_buffer->gpu_start;
	uint64_t end = cur + state->size * 4;

	while (cur < end)
	{
		uint32_t cmd = *(uint32_t *)&state->last_buffer->data[cur];
		int curaddr;
		pushbuf_decode(&state->pstate, cmd, cmdoutput, &curaddr, 1);
		fprintf(stdout, "PB: 0x%08x %s\n", cmd, cmdoutput);

		struct obj *obj = subchans[state->pstate.subchan];
		if (obj)
			demmt_parse_command(obj->class, curaddr, cmd);

		cur += 4;
	}
}

void ib_decode(struct ib_decode_state *state, uint32_t data, char *output)
{
	if ((state->word & 1) == 0)
	{
		if (state->last_buffer)
		{
			ib_print(state);
			state->last_buffer = NULL;
		}

		state->address = data & 0xfffffffc;
		if (data & 0x3)
			mmt_log("invalid ib entry, low2: %d\n", data & 0x3);

		sprintf(output, "IB: addrlow: 0x%08lx", state->address);
	}
	else
	{
		state->address |= ((uint64_t)(data & 0xff)) << 32;
		state->unk8 = (data >> 8) & 0x1;
		state->not_main = (data >> 9) & 0x1;
		state->size = (data & 0x7fffffff) >> 10;
		state->no_prefetch = data >> 31;
		sprintf(output, "IB: address: 0x%08lx, size: %d", state->address, state->size);
		if (state->not_main)
			strcat(output, ", not_main");
		if (state->no_prefetch)
			strcat(output, ", no_prefetch?");
		if (state->unk8)
			strcat(output, ", unk8");

		struct buffer *buf;
		for (buf = buffers_list; buf != NULL; buf = buf->next)
		{
			if (!buf->gpu_start)
				continue;
			if (state->address >= buf->gpu_start && state->address < buf->gpu_start + buf->length)
				break;
		}

		state->last_buffer = buf;
		if (buf)
		{
			char cmdoutput[32];

			sprintf(cmdoutput, ", buffer id: %d", buf->id);
			strcat(output, cmdoutput);
		}
	}
	state->word++;
}

void ib_decode_end(struct ib_decode_state *state)
{
	if (state->last_buffer)
	{
		ib_print(state);
		state->last_buffer = NULL;
	}

	pushbuf_decode_end(&state->pstate);
}
