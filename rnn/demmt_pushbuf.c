/*
 * Copyright (C) 2010 Francisco Jerez.
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

	if (state->size)
		sprintf(output, "size %d, subchannel %d (object handle 0x%x), offset 0x%04x, %s",
				state->size, state->subchan, (obj ? obj->handle : 0), state->addr,
			    (state->incr ? "increment" : "constant"));
	else
		output[0] = 0;
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
		asprintf(&dec_obj, "%s", obj->name);
	else
		asprintf(&dec_obj, "OBJ%X", obj ? obj->class : 0);

	/* get the method name and value */
	if (obj)
	{
		ai = rnndec_decodeaddr(obj->ctx, domain, state->addr, 1);

		dec_addr = ai->name;
		dec_val = rnndec_decodeval(obj->ctx, ai->typeinfo, data, ai->width);

		free(ai);
	}
	else
		asprintf(&dec_addr, "0x%x", state->addr);

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

void pushbuf_decode(struct pushbuf_decode_state *state, uint32_t data, char *output)
{
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
			else
				state->incr = state->size;

			if (mode == 4)
			{
				decode_method(state, state->size, output);
				state->size = 0;
				return;
			}
		}
		else
		{
			if (data & 0x20000000)
			{
				sprintf(output, "# jump to 0x%x", data & 0x1fffffff);
				state->skip = 1;
				return;
			}

			state->addr = data & 0x1fff;
			state->subchan = (data & 0xe000) >> 13;
			state->size = (data & 0x1ffc0000) >> 18;
			state->incr = data & 0x40000000 ? 0 : state->size;
		}

		decode_header(state, data, output);
	}
	else
	{
		if (state->addr == 0 && subchans[state->subchan] == NULL)
			subchans[state->subchan] = get_object(data);

		decode_method(state, data, output);

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
