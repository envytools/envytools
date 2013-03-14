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

#include "dedma.h"

void
parse_renouveau_chipset(struct state *s, char *path)
{
	char *buf = NULL;
	uint32_t val;
	FILE *f;

	asprintf(&buf, "%s/card_stdout.txt", path);
	f = fopen(buf, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open %s: %s\n", buf,
			strerror(errno));
		goto out;
	}

	while (getline(&s->parse.buf, &s->parse.n, f) >= 0) {
		if (sscanf(s->parse.buf, "BOOT0\t%x", &val) == 1) {
			s->chipset = (val & 0x0f000000 ? (val >> 20) & 0xff :
				      val & 0x0000f000 ? ((val >> 20) & 0xf) + 4 :
				      (val >> 16) & 0xf);

		} else if (sscanf(s->parse.buf, "pci_id\t%x", &val) == 1) {
			s->pci_id = val;
		}
	}

	fclose(f);
out:
	free(buf);
}

void
parse_renouveau_objects(struct state *s, char *path)
{
	char *buf = NULL;
	uint32_t handle, ent;
	FILE *f;

	asprintf(&buf, "%s/card_objects.txt", path);
	f = fopen(buf, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open %s: %s\n", buf,
			strerror(errno));
		goto out;
	}

	while (getline(&s->parse.buf, &s->parse.n, f) >= 0) {
		if (sscanf(s->parse.buf, "%x %*x %x", &handle, &ent) == 2)
			add_object(s, handle, (s->chipset >= 0x40 ?
					       ent & 0xffff : ent & 0xfff));
	}

	fclose(f);
out:
	free(buf);
}

void
parse_renouveau_startup(struct state *s, char *path)
{
	char *buf = NULL;
	FILE *f;

	asprintf(&buf, "%s/card_%04x-%04x_test_startup.txt", path,
		 s->pci_id >> 16, s->pci_id & 0xffff);
	f = fopen(buf, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open %s: %s\n", buf,
			strerror(errno));
		goto out;
	}

	dedma(s, f, true);

	fclose(f);
out:
	free(buf);
}

bool
parse_renouveau(struct state *s)
{
	struct ent e = {
		.addr = s->parse.addr
	};
	int ret;

	for (;;) {
		if (getline(&s->parse.buf, &s->parse.n, s->f) < 0)
			return false;

		ret = sscanf(s->parse.buf, "%x", &e.val);
		if (ret == 1)
			break;
	}

	add_ent(s, &e);
	s->parse.addr += 4;
	return true;
}

bool
parse_valgrind(struct state *s)
{
	struct ent e = {};
	unsigned m, addr, val[4];
	char c[3] = {};
	int i;

	for (;;) {
		if (getline(&s->parse.buf, &s->parse.n, s->f) < 0)
			return false;

		i = sscanf(s->parse.buf, "--%*d-- w %d:%x, %x%c%x%c%x%c%x",
			   &m, &addr, &val[0], &c[0], &val[1], &c[1],
			   &val[2], &c[2], &val[3]);
		if(i==6){			//reorder words (2,1) => (1,2)
			unsigned x = val[0];
			val[0] = val[1];
			val[1] = x;
		}
		else if(i==9){		//reorder words (4,3,2,1) => (1,2,3,4)
			unsigned x = val[0];
			val[0] = val[3];
			val[3] = x;
			x      = val[1];
			val[1] = val[2];
			val[2] = x;
		}
		if (i >= 4 && m == s->filter.map)
			break;

		i = sscanf(s->parse.buf, "--%*d-- create gpu object"
			   " 0x%*x:0x%x type 0x%x", &val[0], &val[1]);
		if (i == 2)
			add_object(s, val[0], val[1]);
	}

	for (i = 0; i <= 3 && (i == 0 || c[i - 1] == ','); i++) {
		e.addr = addr;
		e.val = val[i];
		add_ent(s, &e);
		addr += 4;
	}

	return true;
}
