/*
 * Copyright (C) 2013-2014 Martin Peres <martin.peres@labri.fr>
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
#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <sched.h>

 /* we can't use printf because it introduces jitter, let's use this buffer to
  * lower the number of syscalls (writes) and choose when we want them!
  */
static char debugbuf[4096 * 100] = { 0 };
static uint32_t debugoff = 0;
int verbosity = 0;

typedef enum { LOW = 0, DEBUG, ENHANCED, PARANOID } verbosity_level;

typedef enum { false = 0, true = 1 } bool;
enum state {
	IDLE = 0,
	ADDR,
	REG,
	REG_STOP,
	RST_ADDR,
	DATA,
	ERROR
};

struct i2c_state {
	enum state st;

	/* last recorded state of sda and scl */
	uint8_t sda;
	uint8_t scl;

	/* current byte */
	uint8_t byte;
	uint8_t offset;

	/* addr, reg and data associated with the transaction */
	uint8_t addr;
	uint8_t reg;
	uint32_t data;
};

void i2c_init(struct i2c_state *i2c)
{
	i2c->st = IDLE;
	i2c->sda = 1;
	i2c->scl = 1;
	i2c->byte = 0;
	i2c->offset = 0;
	i2c->addr = 0;
	i2c->reg = 0;
	i2c->data = 0;
}

bool i2c_add_data(struct i2c_state *i2c, uint8_t scl, uint8_t sda)
{
	bool ret = false, need_return = false;

	if (i2c->sda == sda && i2c->scl == scl)
		return false;

	if (verbosity >= PARANOID) {
		debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
				     "%i: scl = %i sda = %i: ", i2c->st, scl, sda);
		need_return = true;
	}

	if (i2c->scl && scl) {
		if (i2c->sda && !sda) {
			/* start */
			if (verbosity >= DEBUG) {
				debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
						     " START ");
			}
			if (i2c->st == IDLE)
				i2c->st = ADDR;
			else if (i2c->st == REG_STOP)
				i2c->st = RST_ADDR;
			else {
				if (verbosity >= DEBUG) {
					debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
							     ": ERR");
					need_return = true;
				}
				i2c->st = ERROR;
			}
			i2c->offset = 0;
			i2c->byte = 0;
			i2c->data = 0;
		} else if (!i2c->sda && sda) {
			/* stop */
			if (verbosity >= DEBUG) {
				debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
						     " STOP");
				need_return = true;
			}
			if (i2c->st == DATA && i2c->offset == 1) {
				i2c->st = IDLE;
				ret = true;
			} else {
				if (verbosity >= DEBUG) {
					debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
							     ": ERR, state == %i and offset = %i",
							     i2c->st, i2c->offset);
					need_return = true;
				}
				i2c->st = ERROR;
			}
		}
	} else if (!i2c->scl && scl) {
		if (i2c->offset == 8) {
			if (verbosity >= ENHANCED) {
				debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff, "%i", sda);
			}

			if (i2c->st == ADDR) {
				i2c->addr = i2c->byte;
				i2c->st = REG;
				if (verbosity >= DEBUG) {
					debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
						" Device(0x%.02x) %c ACK(%i) ",
						i2c->addr >> 1, (i2c->addr & 0x1)?'R':'W', sda);
				}
			} else if (i2c->st == REG) {
				i2c->reg = i2c->byte;
				i2c->st = REG_STOP;
				if (verbosity >= DEBUG) {
					debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
							     " Reg(0x%.02x) ACK(%i) ", i2c->reg, sda);
				}
			} else if (i2c->st == RST_ADDR) {
				if (verbosity >= DEBUG) {
					debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
							     " Device(0x%.02x) %c ACK(%i) ",
							     i2c->byte >> 1, (i2c->byte & 1)?'R':'W', sda);
				}
				i2c->addr = i2c->byte;
				if ((i2c->byte & 0xfe) != i2c->addr) {
					i2c->st = ERROR;
				}
				i2c->st = DATA;
			} else if (i2c->st == DATA) {
				if (verbosity >= DEBUG) {
					debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
							     " (0x%.02x) ACK(%i) ", i2c->byte, sda);
				}

				i2c->data <<= 8;
				i2c->data |= i2c->byte;
			}

			i2c->offset = 0;
			i2c->byte = 0;
		} else {
			if (verbosity >= ENHANCED) {
				debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff, "%i", sda);
			}

			i2c->byte |= sda;
			i2c->offset++;
			if (i2c->offset < 8)
				i2c->byte <<= 1;

		}
	}

	if (need_return)
		debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff, "\n");

	i2c->sda = sda;
	i2c->scl = scl;

	return ret;
}

void print_debug()
{
	printf("%s", debugbuf);
	debugoff = 0;
}

void process_set_affinity(int cpu)
{
#ifdef __linux__
	cpu_set_t  mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) < 0)
		perror("sched_setaffinity");
#endif
}

int main(int argc, char **argv)
{
	int c, cnum = 0, port;
	uint8_t scl, sda;
	struct i2c_state i2c;
	int polls = 0;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	while ((c = getopt (argc, argv, "c:a:v")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
			case 'a':
				sscanf(optarg, "%d", &c);
				printf("setting CPU affinity to CPU %i\n", c);
				process_set_affinity(c);
				break;
			case 'v':
				verbosity++;
				break;

		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (optind >= argc) {
		fprintf (stderr, "No i2c port specified.\n");
		return 1;
	}
	sscanf (argv[optind], "%i", &port);

	if (nva_cards[cnum]->chipset.chipset < 0xd9 && port >= 4) {
		printf("Invalid port number: This chipset has 4 ports\n");
		return 1;
	} else if (nva_cards[cnum]->chipset.chipset >= 0xd9 && port >= 10) {
		printf("Invalid port number: GF119+ chipsets have 10 ports\n");
		return 1;
	}

	i2c_init(&i2c);

	printf("device:register (<)=(R/W)=(>) data (exit_status, number of polls during the transaction)\n\n");
	do {
		uint32_t v = nva_rd32(cnum, 0xd000 + port * 0x20 + 0x14);
		if (v != 0x37)
			polls++;

		scl = !!(v & 0x10);
		sda = !!(v & 0x20);

		bool ret = i2c_add_data(&i2c, scl, sda);
		if (ret || i2c.st == ERROR) {
			debugoff += snprintf(debugbuf + debugoff, sizeof(debugbuf) - debugoff,
					     "%.02x:%.02x %s %.04x (status = %i, polls = %.04i)%s\n",
					     i2c.addr, i2c.reg, (i2c.addr & 1)?"=R=>":"<=W=",
					     i2c.data, i2c.st, polls, (i2c.st == ERROR)?": ERR":"");
			print_debug();
			polls = 0;
			i2c_init(&i2c);
		}
		sched_yield();
	} while (1);

	return 0;
}
