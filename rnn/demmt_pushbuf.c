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
#include "demmt_nvrm.h"

void pushbuf_decode_start(struct pushbuf_decode_state *state)
{
	memset(state, 0, sizeof(*state));
}

struct obj *subchans[8] = { NULL };
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
		obj->name = 0;
		obj->ctx = rnndec_newcontext(rnndb);
		obj->ctx->colors = colors;
		obj->decoder = demmt_get_decoder(class);

		v = NULL;
		FINDARRAY(chs->vals, v, v->value == (uint64_t)chipset);
		rnndec_varadd(obj->ctx, "chipset", v ? v->name : "NV01");

		v = NULL;
		FINDARRAY(cls->vals, v, v->value == class);
		obj->desc = v ? v->name : NULL;
		rnndec_varadd(obj->ctx, "obj-class", v ? v->name : "NV01_NULL");

		return;
	}

	fprintf(stderr, "Too many objects\n");
	abort();
}

void pushbuf_add_object_name(uint32_t handle, uint32_t name)
{
	struct obj *objs = objects;
	int i;
	for (i = 0; i < MAX_OBJECTS; i++)
		if (objs[i].handle == handle)
		{
			objs[i].name = name;
			return;
		}

	mmt_error("pushbuf_add_object_name(0x%08x, 0x%08x): no object\n", handle, name);
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

	for (i = 0; i < MAX_OBJECTS; i++)
		if (objs[i].name == handle)
			return &objs[i];

	if (chipset >= 0xc0)
	{
		mmt_error("Guessing handle 0x%08x, driver forgot to call NVRM_MTHD_FIFO_IB_OBJECT_INFO?\n", handle);
		pushbuf_add_object(handle, handle & 0xffff);
		return get_object(handle);
	}

	return NULL;
}

static void decode_header(struct pushbuf_decode_state *state, char *output)
{
	struct obj *obj = subchans[state->subchan];
	uint32_t handle = obj ? obj->handle : 0;
	const char *incr = state->incr ? "increment" : "constant";
	char subchannel_desc[128];

	if (obj)
	{
		const char *name = demmt_nvrm_get_class_name(obj->class);
		if (name)
			sprintf(subchannel_desc, " (class: 0x%04x, desc: %s, handle: 0x%08x)", obj->class, name, handle);
		else
			sprintf(subchannel_desc, " (class: 0x%04x, handle: 0x%08x)", obj->class, handle);
	}
	else
		subchannel_desc[0] = 0;


	if (!state->long_command)
		sprintf(output, "size %d, subchannel %d%s, offset 0x%04x, %s",
				state->size, state->subchan, subchannel_desc, state->addr, incr);
	else
		sprintf(output, "size ?, subchannel %d%s, offset 0x%04x, %s",
				state->subchan, subchannel_desc, state->addr, incr);
}

void decode_method_raw(int mthd, uint32_t data, struct obj *obj, char *dec_obj,
		char *dec_mthd, char *dec_val)
{
	/* get an object name */
	if (obj && obj->desc)
		sprintf(dec_obj, "%s%s%s", colors->rname, obj->desc, colors->reset);
	else
		sprintf(dec_obj, "%sOBJ%X%s", colors->err, obj ? obj->class : 0, colors->reset);

	/* get the method name and value */
	if (obj)
	{
		char *tmp;
		struct rnndecaddrinfo *ai;
		int bucket = (mthd * (mthd + 3)) % ADDR_CACHE_SIZE;
		struct cache_entry *entry = obj->cache[bucket];
		while (entry && entry->mthd != mthd)
			entry = entry->next;
		if (entry)
			ai = entry->info;
		else
		{
			entry = malloc(sizeof(struct cache_entry));
			entry->mthd = mthd;
			entry->info = ai = rnndec_decodeaddr(obj->ctx, domain, mthd, 1);
			entry->next = obj->cache[bucket];
			obj->cache[bucket] = entry;
		}

		strcpy(dec_mthd,  ai->name);
		if (dec_val)
		{
			tmp = rnndec_decodeval(obj->ctx, ai->typeinfo, data, ai->width);
			if (tmp)
			{
				strcpy(dec_val, tmp);
				free(tmp);
			}
			else
				sprintf(dec_val, "%s0x%x%s", colors->err, data, colors->reset);
		}
	}
	else
	{
		sprintf(dec_mthd, "%s0x%x%s", colors->err, mthd, colors->reset);
		if (dec_val)
			sprintf(dec_val, "%s0x%x%s", colors->err, data, colors->reset);
	}
}

static void decode_method(struct pushbuf_decode_state *state, char *output)
{
	struct obj *obj = subchans[state->subchan];
	static char dec_obj[1000], dec_mthd[1000], dec_val[1000];

	decode_method_raw(state->mthd, state->mthd_data, obj, dec_obj, dec_mthd, dec_val);

	if (state->mthd == 0)
		sprintf(output, "  %s mapped to subchannel %d", dec_obj, state->subchan);
	else
		sprintf(output, "  %s.%s = %s", dec_obj, dec_mthd, dec_val);
}

/* returns 0 when decoding should continue, anything else: next command gpu address */
uint64_t pushbuf_decode(struct pushbuf_decode_state *state, uint32_t data, char *output, int safe)
{
	state->mthd = -1;
	state->mthd_data_available = 0;

	if (state->skip)
	{
		strcpy(output, "SKIP");
		state->skip--;
		return 0;
	}

	if (state->size == 0)
	{
		if (data == 0 && !state->long_command)
		{
			strcpy(output, "NOP");
			return 0;
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
				state->mthd = state->addr;
				state->mthd_data_available = 1;
				state->mthd_data = state->size;
				decode_method(state, output);
				state->size = 0;
				return 0;
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
					return 0;
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
					return 0;
				}

				if (!state->pushbuf_invalid)
					mmt_log("unusual, old-style non-inc mthd%s\n", "");
			}
			else
			{
				state->size = 0;
				sprintf(output, "unknown mode %d", mode);
				return 0;
			}
		}
		else
		{
			if (state->long_command)
			{
				state->size = data & 0xffffff;
				state->long_command = 0;
				sprintf(output, "size %d", state->size);
				return 0;
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
						return 0;
					}
					else if (type == 2)
					{
						state->size = 0;
						sprintf(output, "return");
						return 0;
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
					return 1;
				}
				else if (mode == 2)
					state->incr = 0;
				else
				{
					sprintf(output, "unknown mode, top 3 bits: %d", mode);
					state->size = 0;
					return 0;
				}
			}
			else if (type == 1)
			{
				uint32_t addr = data & 0xfffffffc;
				sprintf(output, "jump to 0x%x", addr);
				state->size = 0;
				return addr;
			}
			else if (type == 2)
			{
				sprintf(output, "call 0x%x", data & 0xfffffffc);
				state->size = 0;
				return 0; // XXX
			}
			else
			{
				sprintf(output, "unknown type, bottom 2 bits: %d", type);
				state->size = 0;
				return 0;
			}
		}

		state->mthd = state->addr;
		decode_header(state, output);
		if (chipset >= 0xe0 && subchans[state->subchan] == NULL)
		{
			uint32_t handle = 0;
			if (state->subchan == 0)
			{
				if (chipset == 0xea)
					handle = 0xa297;
				else if (chipset < 0xf0)
					handle = 0xa097;
				else if (chipset < 0x110)
					handle = 0xa197;
				else
					handle = 0xb097;
			}
			else if (state->subchan == 1)
			{
				if (chipset < 0xf0)
					handle = 0xa0c0;
				else if (chipset < 0x100)
					handle = 0xa1c0;
				else
					handle = 0xb0c0;
			}
			else if (state->subchan == 2)
			{
				if (chipset < 0xf0)
					handle = 0xa040;
				else
					handle = 0xa140;
			}
			else if (state->subchan == 3)
				handle = 0x902d;
			else if (state->subchan == 4)
			{
				if (chipset < 0x100)
					handle = 0xa0b5;
				else
					handle = 0xb0b5;
			}

			if (handle)
				subchans[state->subchan] = get_object(handle);
		}
		if (guess_invalid_pushbuf && subchans[state->subchan] == NULL && state->addr != 0 && state->pushbuf_invalid == 0)
		{
			mmt_log("subchannel %d does not have bound object and first command does not bind it, marking this buffer invalid\n", state->subchan);
			state->pushbuf_invalid = 1;
		}
	}
	else
	{
		state->mthd = state->addr;
		state->mthd_data_available = 1;
		state->mthd_data = data;

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

		decode_method(state, output);

		if (state->incr)
		{
			state->addr += 4;
			state->incr--;
		}

		state->size--;
	}

	return 0;
}

void pushbuf_decode_end(struct pushbuf_decode_state *state)
{
}


void ib_decode_start(struct ib_decode_state *state)
{
	memset(state, 0, sizeof(*state));
	pushbuf_decode_start(&state->pstate);
}

uint64_t pushbuf_print(struct pushbuf_decode_state *pstate, struct buffer *buffer, uint64_t gpu_address, int commands)
{
	char cmdoutput[1024];
	uint64_t cur = gpu_address - buffer->gpu_start;
	uint64_t end = cur + commands * 4;
	uint64_t nextaddr;

	while (cur < end)
	{
		uint32_t cmd = *(uint32_t *)&buffer->data[cur];
		nextaddr = pushbuf_decode(pstate, cmd, cmdoutput, 1);
		if (nextaddr)
		{
			mmt_log("decoding aborted, cmd: \"%s\", nextaddr: 0x%08lx\n", cmdoutput, nextaddr);
			return nextaddr;
		}
		fprintf(stdout, "PB: 0x%08x %s", cmd, cmdoutput);

		struct obj *obj = subchans[pstate->subchan];

		if (obj)
		{
			if (obj->data == NULL)
			{
				obj->data = calloc(1, sizeof(struct buffer));
				obj->data->id = -1;
				obj->data->length = OBJECT_SIZE;
				obj->data->data = calloc(obj->data->length, 1);
			}

			if (pstate->mthd_data_available)
			{
				if (pstate->mthd < OBJECT_SIZE)
					buffer_register_write(obj->data, pstate->mthd, 4, &pstate->mthd_data);
				else
					mmt_log("not enough space for object data 0x%x\n", pstate->mthd);

				if (decode_object_state && obj->decoder && obj->decoder->decode_terse)
					obj->decoder->decode_terse(pstate);
			}
		}

		fprintf(stdout, "\n");

		if (decode_object_state && pstate->mthd_data_available && obj && obj->decoder && obj->decoder->decode_verbose)
			obj->decoder->decode_verbose(pstate);

		cur += 4;
	}

	return gpu_address + commands * 4;
}

static void ib_print(struct ib_decode_state *state)
{
	pushbuf_print(&state->pstate, state->last_buffer, state->address, state->size);
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
		else
			strcat(output, ", no such buffer!");
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

void user_decode_start(struct user_decode_state *state)
{
	memset(state, 0, sizeof(*state));
	pushbuf_decode_start(&state->pstate);
}

static void user_print(struct user_decode_state *state)
{
	struct buffer *buf = state->last_buffer;
	if (state->dma_put < state->prev_dma_put)
	{
		uint64_t nextaddr = pushbuf_print(&state->pstate, buf, state->prev_dma_put,
										(buf->gpu_start + buf->length - state->prev_dma_put) / 4);
		if (state->dma_put >= buf->gpu_start && state->dma_put < buf->gpu_start + buf->length &&
				  nextaddr >= buf->gpu_start &&       nextaddr < buf->gpu_start + buf->length &&
				  nextaddr <= state->dma_put &&
				  nextaddr <= 0xffffffff)
		{
			mmt_log("pushbuffer wraparound%s\n", "\n");
			state->prev_dma_put = nextaddr;
		}
		else
		{
			mmt_log("confused, dma_put: 0x%x, nextaddr: 0x%lx, buffer: <0x%08lx,0x%08lx>, resetting state\n",
					state->dma_put, nextaddr, buf->gpu_start, buf->gpu_start + buf->length);
			state->last_buffer = NULL;
			state->prev_dma_put = state->dma_put;
			return;
		}
	}

	pushbuf_print(&state->pstate, buf, state->prev_dma_put, (state->dma_put - state->prev_dma_put) / 4);
	state->prev_dma_put = state->dma_put;
}

void user_decode(struct user_decode_state *state, uint32_t addr, uint32_t data, char *output)
{
	struct buffer *buf = state->last_buffer;
	if (buf && state->prev_dma_put != state->dma_put)
		user_print(state);

	if (addr != 0x40) // DMA_PUT
	{
		output[0] = 0;
		return;
	}

	if (buf)
		if (data < buf->gpu_start || data >= buf->gpu_start + buf->length)
			buf = NULL;

	if (!buf)
	{
		for (buf = buffers_list; buf != NULL; buf = buf->next)
		{
			if (!buf->gpu_start)
				continue;
			if (data >= buf->gpu_start && data < buf->gpu_start + buf->length)
			{
				state->prev_dma_put = buf->gpu_start;
				break;
			}
		}
	}

	state->last_buffer = buf;
	if (buf)
		state->dma_put = data;

	sprintf(output, "DMA_PUT: 0x%08x", data);
	if (buf)
	{
		char cmdoutput[32];

		sprintf(cmdoutput, ", buffer id: %d", buf->id);
		strcat(output, cmdoutput);
	}
}

void user_decode_end(struct user_decode_state *state)
{
	if (state->last_buffer && state->prev_dma_put != state->dma_put)
		user_print(state);

	pushbuf_decode_end(&state->pstate);
}
