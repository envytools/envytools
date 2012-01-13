/*
 * Copyright (C) 2010 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "dedma.h"
#include "util.h"

void
add_object(struct state *s, uint32_t handle, uint32_t class)
{
	struct rnnenum *chs = rnn_findenum(s->db, "chipset");
	struct rnnenum *cls = rnn_findenum(s->db, "obj-class");
	struct rnnvalue *v;
	struct obj *obj;
	int i;

	if (!cls || !chs) {
		fprintf(stderr, "No obj-class/chipset enum found\n");
		abort();
	}

	for (i = 0; obj = &s->objects[i], i < MAX_OBJECTS; i++) {
		if (!obj->handle) {
			obj->handle = handle;
			obj->class = class;
			obj->ctx = rnndec_newcontext(s->db);
			obj->ctx->colors = s->colors;

			v = NULL;
			FINDARRAY(chs->vals, v, v->value == s->chipset);
			rnndec_varadd(obj->ctx, "chipset",
				      v ? v->name : "NV01");

			v = NULL;
			FINDARRAY(cls->vals, v, v->value == class);
			obj->name = v ? v->name : NULL;
			rnndec_varadd(obj->ctx, "obj-class",
				      v ? v->name : "NV01_NULL");

			return;
		}
	}

	fprintf(stderr, "Too many objects\n");
}

struct obj *
get_object(struct state *s, uint32_t handle)
{
	struct obj *objs = s->objects;
	int i;

	for (i = 0; i < MAX_OBJECTS; i++) {
		if (objs[i].handle == handle)
			return &objs[i];
	}

	if (s->chipset >= 0xc0) {
		add_object(s, handle, handle & 0xffff);
		return get_object(s, handle);
	}

	return NULL;
}

static void
flush_raw(struct state *s, struct ent *e)
{
	if (e->out_of_band)
		s->op.print("# out of band: %08x %08lx\n", e->addr, e->val);
	else
		s->op.print("%08x %08lx\n", e->addr, e->val);
}

static void
pretty_header(struct state *s, struct ent *e)
{
	struct dma *dma = &s->dma;
	struct obj *obj = s->subchan[dma->subchan];

	if (dma->size)
		s->op.print("%08x  size %d, subchannel %d (0x%x),"
			    " offset 0x%04x, %s\n", e->val, dma->size,
			    dma->subchan, (obj ? obj->handle : 0), dma->addr,
			    (dma->incr ? "increment" : "constant"));
	else
		s->op.print("%08x\n", e->val);
}

static void
pretty_method(struct state *s, struct ent *e, uint32_t x)
{
	struct rnndecaddrinfo *ai;
	struct rnndeccolors *col = s->colors;
	struct dma *dma = &s->dma;
	struct obj *obj = s->subchan[dma->subchan];
	char *dec_obj = NULL;
	char *dec_addr = NULL;
	char *dec_val = NULL;

	/* get an object name */
	if (obj && obj->name)
		asprintf(&dec_obj, "%s%s%s", col->cname,
			 obj->name, col->cend);
	else
		asprintf(&dec_obj, "%sOBJ%X%s", col->cerr,
			 (obj ? obj->class : 0), col->cend);

	/* get the method name and value */
	if (obj) {
		ai = rnndec_decodeaddr(obj->ctx, s->dom, dma->addr, true);

		dec_addr = ai->name;
		dec_val = rnndec_decodeval(obj->ctx, ai->typeinfo, x,
					   ai->width);

		free(ai);
	} else {
		asprintf(&dec_addr, "%s0x%x%s", col->cerr, dma->addr,
			 col->cend);
	}

	/* write it */
	s->op.print("%08x    %s", e->val, dec_obj);

	if (dma->addr == 0)
		s->op.print(" mapped to subchannel %d\n", dma->subchan);
	else if (dec_val)
		s->op.print(".%s = %s\n", dec_addr, dec_val);
	else
		s->op.print(".%s\n", dec_addr);

	free(dec_val);
	free(dec_addr);
	free(dec_obj);
}

static void
flush_dma(struct state *s, struct ent *e)
{
	struct dma *dma = &s->dma;
	uint32_t x = e->val;

	if (e->out_of_band) {
		s->op.print("# out of band: %08x %08lx\n", e->addr, e->val);
		return;
	}

	if (dma->skip) {
		s->op.print("%08x\n", x);
		dma->skip--;
		return;
	}

	if (!dma->size) {
		if (!x) {
			dma->nops++;
			return;

		} else if (dma->nops) {
			s->op.print("# %d NOPs\n", dma->nops);
			dma->nops = 0;
		}

		if (s->chipset >= 0xc0) {
			int mode = (x & 0xe0000000) >> 29;

			dma->addr = (x & 0x1fff) << 2;
			dma->subchan = (x & 0xe000) >> 13;
			dma->size = (x & 0x1fff0000) >> 16;
			dma->incr = (mode == 3 ? 0 :
				     mode == 5 ? 1 :
				     dma->size);

			if (mode == 4) {
				pretty_method(s, e, dma->size);
				dma->size = 0;
				return;
			}
		} else {
			if (x & 0x20000000) {
				s->op.print("# jump to 0x%x: %08x %08lx\n",
					     x & 0x1fffffff, e->addr, e->val);
				dma->skip = 1;
				return;
			}

			dma->addr = x & 0x1fff;
			dma->subchan = (x & 0xe000) >> 13;
			dma->size = (x & 0x1ffc0000) >> 18;
			dma->incr = (x & 0x40000000 ? 0 : dma->size);
		}

		pretty_header(s, e);
	} else {
		if (dma->addr == 0)
			s->subchan[dma->subchan] = get_object(s, x);

		pretty_method(s, e, x);

		if (dma->incr) {
			dma->addr += 4;
			dma->incr--;
		}

		dma->size--;
	}
}

static int
dont_printf(const char *fmt, ...)
{
	return 0;
}

int
dedma(struct state *s, FILE *f, bool dry_run)
{
	struct filter *flt = &s->filter;
	struct ent *e0 = NULL, *e1;
	int i, j; /* dump line numbers */

	s->f = f;
	s->op.print = (dry_run ? dont_printf : printf);

	for (i = 0; e1 = e0, e0 = get_ent(s, i); i++) {
		if (!e1)
			continue;

		if (e0->addr < flt->addr0 || e0->addr >= flt->addr1) {
			/* out of band data */
			e0->out_of_band = true;

		} else if (abs(e0->addr - e1->addr) > MAX_DELTA &&
			   !e1->out_of_band) {
			/* wrap around or out of band data not caught by
			 * the address window */

			for (j = i + 1; (j < i + MAX_OUT_OF_BAND &&
					 (e1 = get_ent(s, j))); j++) {
				if (abs(e0->addr - e1->addr) > MAX_DELTA) {
					for (; i < j; i++)
						get_ent(s, i)->out_of_band = true;

					/* shrink the address window */
					if (e1->addr > e0->addr)
						flt->addr0 = max(e0->addr + 4,
								 flt->addr0);
					else if (e1->addr < e0->addr)
						flt->addr1 = min(e0->addr,
								 flt->addr1);
					break;
				}
			}

		} else if (e0->addr < e1->addr + 4) {
			/* badly ordered write, find a place for it */

			for (j = i - 1; (j > i - MAX_DELTA &&
					 (e1 = get_ent(s, j))); j--) {
				if (e0->addr == e1->addr) {
					/* duplicated write, keep the
					 * most recent one */
					*e1 = *e0;
					drop_ent(s, i--);
					break;

				} else if (e0->addr == e1->addr + 3) {
					/* 8bit writes */
					e1->val |= e0->val << 24;
					drop_ent(s, i--);
					break;

				} else if (e0->addr == e1->addr + 2) {
					/* 16bit writes */
					e1->val |= e0->val << 16;
					drop_ent(s, i--);
					break;

				} else if (e0->addr == e1->addr + 1) {
					/* 8bit writes */
					e1->val |= e0->val << 8;
					drop_ent(s, i--);
					break;

				} else if (e0->addr > e1->addr) {
					/* unordered write */
					rotate_ent(s, j + 1, i);
					break;
				}
			}
		}
	}

	flush_cache(s);
	return 0;
}

static bool
configure(struct state *s, int argc, char *argv[], char **path,
	  struct obj obj[MAX_OBJECTS], int *nobj)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-x")) {
			s->op.flush = flush_raw;

		} else if (!strcmp(argv[i], "-c")) {
			s->colors = &rnndec_colorsterm;

		} else if (!strcmp(argv[i], "-m")) {
			if (i + 1 >= argc)
				goto fail;

			if (!strncasecmp(argv[++i], "NV", 2))
				s->chipset = strtoul(argv[i] + 2, NULL, 16);
			else
				s->chipset = strtoul(argv[i], NULL, 16);

		} else if (!strcmp(argv[i], "-o")) {
			if (i + 2 >= argc || *nobj == MAX_OBJECTS)
				goto fail;

			obj[*nobj].handle = strtoul(argv[++i], NULL, 16);
			obj[*nobj].class = strtoul(argv[++i], NULL, 16);
			(*nobj)++;

		} else if (!strcmp(argv[i], "-r")) {
			if (i + 1 >= argc)
				goto fail;

			s->op.parse = parse_renouveau;
			*path = argv[++i];

		} else if (!strcmp(argv[i], "-v")) {
			if (i + 2 >= argc)
				goto fail;

			s->op.parse = parse_valgrind;
			s->filter.map = strtol(argv[++i], NULL, 10);
			*path = argv[++i];

		} else {
			goto fail;
		}
	}

	if (!*path)
		goto fail;
	if (!s->colors)
		s->colors = &rnndec_colorsnull;
	if (!s->op.flush)
		s->op.flush = flush_dma;
	s->filter.addr1 = ~0;

	return true;
fail:
	fprintf(stderr, "usage: %s [ -x ] [ -c ] [ -m 'chipset' ]"
		" [ -o 'handle' 'class' ] [ -r 'file' ] [ -v 'map' 'file' ]\n"
		"\t-x\tHexadecimal output mode.\n"
		"\t-c\tClassy output mode.\n"
		"\t-m\tForce chipset version.\n"
		"\t-o\tForce handle to class mapping"
		" (repeat for multiple mappings).\n"
		"\t-r\tParse a renouveau trace.\n"
		"\t-v\tParse a valgrind-mmt trace.\n",
		argv[0]);
	return false;
}

int
main(int argc, char *argv[])
{
	struct state s = { };
	struct obj obj[MAX_OBJECTS];
	int nobj = 0;
	char *path = NULL;
	FILE *f;
	int i;

	/* parse the command line */
	if (!configure(&s, argc, argv, &path, obj, &nobj))
		return EINVAL;

	/* open the input */
	f = fopen(path, "r");
	if (!f) {
		perror(argv[0]);
		return errno;
	}

	/* set up an rnn context */
	rnn_init();
	s.db = rnn_newdb();
	rnn_parsefile(s.db, "nv_objects.xml");
	rnn_prepdb(s.db);
	s.dom = rnn_finddomain(s.db, "NV01_SUBCHAN");

	/* insert objects specified in the command line */
	for (i = 0; i < nobj; i++)
		add_object(&s, obj[i].handle, obj[i].class);

	if (s.op.parse == parse_renouveau) {
		path = dirname(path);
		parse_renouveau_chipset(&s, path);
		parse_renouveau_objects(&s, path);
		parse_renouveau_startup(&s, path);
	}

	/* do it */
	dedma(&s, f, false);

	/* clean up */
	fclose(f);
	free(s.parse.buf);

	return 0;
}
