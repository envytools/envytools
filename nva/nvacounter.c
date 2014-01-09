/*
 * Copyright (C) 2011 Martin Peres <martin.peres@ensi-bourges.fr>
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

#include <pciaccess.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "nva.h"
#include "rnn.h"

#define SIGNALS_LENGTH 0x100
#define SIGNAL_VALUE(_signal, _set, _index) ((_signal)[(_set) * (0x20 * 8) + (_index)])
uint8_t signals_ref[SIGNALS_LENGTH * 8]; // reserve an octet for each bit of the a800 range

uint64_t get_current_time_us()
{
        struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + (ts.tv_nsec / 1000);
}

void poll_signals(int cnum, uint8_t *signals)
{
	int i, j, b;

	for (i = 0; i < SIGNALS_LENGTH * 8; i++)
		signals[i] = 0;

	for (i = 0; i < 100; i++) {
		for (j = 0; j < SIGNALS_LENGTH; j+=4) {
			uint32_t value = nva_rd32(cnum, 0xa800 + j);

			for (b = 0; b < 32; b++)
				if (value & (1 << b))
					signals[j * 8 + b]++;
		}
	}
}

void nv40_start_monitoring(int cnum, int set, int s1, int s2, int s3, int s4)
{
	nva_wr32(cnum, 0xa7c0 + set * 4, 0x1);
	nva_wr32(cnum, 0xa500 + set * 4, 0);
	nva_wr32(cnum, 0xa520 + set * 4, 0);

	nva_wr32(cnum, 0xa400 + set * 4, s1);
	nva_wr32(cnum, 0xa440 + set * 4, s2);
	nva_wr32(cnum, 0xa480 + set * 4, s3);
	nva_wr32(cnum, 0xa4c0 + set * 4, s4);

	nva_wr32(cnum, 0xa420 + set * 4, 0xaaaa);
	nva_wr32(cnum, 0xa460 + set * 4, 0xaaaa);
	nva_wr32(cnum, 0xa4a0 + set * 4, 0xaaaa);
	nva_wr32(cnum, 0xa4e0 + set * 4, 0xaaaa);

	/* reset the counters */
	nva_mask(cnum, 0x400084, 0x20, 0x20);
}

void nv40_stop_monitoring(int cnum, int set, uint8_t *signals, uint32_t *real_signals, uint32_t *cycles)
{
	uint32_t cycles_cnt, s[4];
	uint8_t w_s[4];
	int i;

	/* readout */
	nva_mask(cnum, 0x400084, 0x0, 0x20);

	w_s[0] = nva_rd32(cnum, 0xa400 + set * 4);
	w_s[1] = nva_rd32(cnum, 0xa440 + set * 4);
	w_s[2] = nva_rd32(cnum, 0xa480 + set * 4);
	w_s[3] = nva_rd32(cnum, 0xa4c0 + set * 4);

	cycles_cnt = nva_rd32(cnum, 0xa600 + set * 4);
	s[0] = nva_rd32(cnum, 0xa700 + set * 4);
	s[1] = nva_rd32(cnum, 0xa6c0 + set * 4);
	s[2] = nva_rd32(cnum, 0xa680 + set * 4);
	s[3] = nva_rd32(cnum, 0xa740 + set * 4);

	for (i = 0; i < 4; i++) {
		if (signals)
			SIGNAL_VALUE(signals, set, w_s[i]) = s[i] * 100 / cycles_cnt;
		if (real_signals)
			SIGNAL_VALUE(real_signals, set, w_s[i]) = s[i];
		if (cycles)
			SIGNAL_VALUE(cycles, set, w_s[i]) = cycles_cnt;
	}
}


void print_poll_data(const char *name, uint8_t *signals)
{
	int i, s;

	printf("Print data poll '%s':\n", name);
	for (s = 0; s < 8; s++) {
		printf("Set %i:", s);
		for (i = 0; i < 0x20 * 8; i++) {
			if (i % 0x10 == 0)
				printf("\n0x%02x: ", i % (0x20 * 8));

			printf("%.3i ", SIGNAL_VALUE(signals, s, i));

			if (i != 0 && i % (SIGNALS_LENGTH) == 0)
				printf("\n\n");
		}

		if (s != 7)
			printf("\n");
		printf("\n");
	}
}

void offset_to_set_and_signal(int offset, uint8_t *set, uint8_t *signal)
{
	if (set)
		*set = offset / 0x100;
	if (signal)
		*signal = offset % 0x100;
}

struct signals_comparaison
{
	int diff_count;
	struct signal_difference
	{
		uint8_t set;
		uint8_t signal;
		enum signals_change_type{
			NO_CHANGE = -1,
			ONE_TO_ZERO = 0,
			ZERO_TO_ONE = 1,
			OTHER_TO_ZERO,
			OTHER_TO_ONE,
			ONE_TO_OTHER,
			ZERO_TO_OTHER
		} change;
		uint8_t count_before;
		uint8_t count_after;
		uint8_t count_total;
	} *differences;
};

void print_difference(struct signal_difference diff)
{
	const char *difference_s = NULL;

	switch(diff.change)
	{
	case NO_CHANGE:
		difference_s = "NO CHANGE";
		break;
	case ONE_TO_ZERO:
		difference_s = "ONE TO ZERO";
		break;
	case ZERO_TO_ONE:
		difference_s = "ZERO TO ONE";
		break;
	case OTHER_TO_ZERO:
		difference_s = "OTHER TO ZERO";
		break;
	case OTHER_TO_ONE:
		difference_s = "OTHER TO ONE";
		break;
	case ONE_TO_OTHER:
		difference_s = "ONE TO OTHER";
		break;
	case ZERO_TO_OTHER:
		difference_s = "ZERO TO OTHER";
		break;
	}
	printf("Diff at set %i, signal 0x%.2x): %i->%i/100 (%s?)\n",
			      diff.set, diff.signal, diff.count_before, diff.count_after, difference_s);
}

int signals_normalise(uint8_t value)
{
	if (value < 10)
		return 0;
	else if (value > 90)
		return 1;
	else
		return -1;
}

struct signals_comparaison signals_compare(unsigned char *one, unsigned char *two)
{
	struct signals_comparaison compare;
	int i;

	compare.diff_count = 0;
	compare.differences = realloc(NULL, 10 * sizeof(struct signal_difference));

	for (i = 0; i < SIGNALS_LENGTH * 8; i++) {
		int before = signals_normalise(one[i]);
		int after = signals_normalise(two[i]);
		enum signals_change_type change = NO_CHANGE;

		if (before != after) {
			uint8_t set, signal;

			if (before == 1 && after == 0)
				change = ONE_TO_ZERO;
			else if (before == 0 && after == 1)
				change = ZERO_TO_ONE;
			else if (before == -1 && after == 0)
				change = OTHER_TO_ZERO;
			else if (before == -1 && after == 1)
				change = OTHER_TO_ONE;
			else if (before == 1 && after == -1)
				change = ONE_TO_OTHER;
			else if (before == 0 && after == -1)
				change = ZERO_TO_OTHER;

			offset_to_set_and_signal(i, &set, &signal);

			struct signal_difference diff;
			diff.set = set;
			diff.signal = signal;
			diff.change = change;
			diff.count_before = one[i];
			diff.count_after = two[i];

			compare.differences = realloc(compare.differences,
						      (compare.diff_count+1) * sizeof(struct signal_difference));
			compare.differences[compare.diff_count] = diff;
			compare.diff_count++;
		}
	}

	return compare;
}

void signals_comparaison_free(struct signals_comparaison comparaison)
{
	free(comparaison.differences);
	comparaison.differences = NULL;
}

void find_ptimer_b12(int cnum)
{
	uint8_t signals_ref_timer[0x100 * 8];
	uint8_t signals_tmp[0x100 * 8];
	struct signals_comparaison diffs;
	int i;
	uint32_t r_9210, r_9400;

	printf("<PTIMER_B12>\n");

	r_9210 = nva_rd32(cnum, 0x9210);
	r_9400 = nva_rd32(cnum, 0x9400);

	/* stop the time */
	nva_wr32(cnum, 0x9210, 0);
	nva_wr32(cnum, 0x9400, 0);

	poll_signals(cnum, signals_ref_timer);
	nva_wr32(cnum, 0x9400, 0x20000);
	poll_signals(cnum, signals_tmp);

	/* restore ptimer */
	nva_wr32(cnum, 0x9400, r_9400);
	nva_wr32(cnum, 0x9210, r_9210);

	diffs = signals_compare(signals_ref_timer, signals_tmp);

	if (diffs.diff_count >= 1) {
		for (i = 0; i < diffs.diff_count; i++) {
			uint8_t set, signal;

			set = diffs.differences[i].set;
			signal = diffs.differences[i].signal;

			if (diffs.differences[i].set == 0) {
				if (diffs.differences[i].change == ZERO_TO_ONE)
					printf("PTIMER_B12: Set %u, signal 0x%.2x\n", set, signal);
			} else {
				printf("Unexpected difference: ");
				print_difference(diffs.differences[i]);
			}
		}
	} else
		printf("Not found.\n");

	signals_comparaison_free(diffs);

	printf("</PTIMER_B12>\n\n");
}

void print_counter_value(const char *name, char *subname, int set,
			 int signal, uint32_t val, uint32_t cycles)
{
	printf("%s_%s: Set %u, signal 0x%x: %u/%u --> %.03f%%\n",
		name, subname,
		set, signal, val, cycles,
		val * 100.0 / cycles);
}

void find_counter_noise(int cnum)
{
	uint32_t signals_real[0x100 * 8] = { 0 };
	uint32_t signals_real_cycles[0x100 * 8] = { 0 };
	int i;

	printf("<PCOUNTER_NOISE: 1ms wait> /!\\ no drivers should be loaded!\n");

	/* bare minimum activity  */
	for (i = 0; i < 0xff; i+=4) {
		nv40_start_monitoring(cnum, 0, i, i+1, i+2, i+3);
		usleep(1000);
		nv40_stop_monitoring(cnum, 0, NULL, signals_real, signals_real_cycles);
	}

	for (i = 0; i < 0xff; i++) {
		uint32_t val = SIGNAL_VALUE(signals_real, 0, i);
		uint32_t cycles = SIGNAL_VALUE(signals_real_cycles, 0, i);
		if (val > 0)
			print_counter_value("NOISE", "UNK", 0, i, val, cycles);
	}

	printf("</PCOUNTER_NOISE>\n\n");
}

void find_mmio_read_write(int cnum, uint32_t reg, const char *engine)
{
	uint32_t signals_real[0x100 * 8] = { 0 };
	uint32_t signals_real_cycles[0x100 * 8] = { 0 };
	uint32_t reg_val;
	int i, e;

	printf("<%s (1000 rd, 200 wr @ 0x%x)> /!\\ no drivers should be loaded!\n", engine, reg);

	/* let's create some activity  */
	for (i = 0; i < 0xff; i+=4) {
		nv40_start_monitoring(cnum, 0, i, i+1, i+2, i+3);
		for (e = 0; e < 1000; e++)
			reg_val = nva_rd32(cnum, reg);
		for (e = 0; e < 200; e++)
			nva_wr32(cnum, reg, reg_val);
		nv40_stop_monitoring(cnum, 0, NULL, signals_real, signals_real_cycles);
	}

	for (i = 0; i < 0xff; i++) {
		uint32_t val = SIGNAL_VALUE(signals_real, 0, i);
		uint32_t cycles = SIGNAL_VALUE(signals_real_cycles, 0, i);
		if (val == 201)
			print_counter_value(engine, "WR", 0, i, val, cycles);
		else if (val == 1001)
			print_counter_value(engine, "RD", 0, i, val, cycles);
		else if (val == 1202)
			print_counter_value(engine, "TRANS", 0, i, val, cycles);
		else if (val == 3003)
			print_counter_value(engine, "3_RD", 0, i, val, cycles);
		else if (val >= 10 && val < 10000)
			print_counter_value("UNK", "", 0, i, val, cycles);
	}

	printf("</%s>\n\n", engine);
}

void find_host_mem_read_write(int cnum)
{
	uint32_t signals_real[0x100 * 8] = { 0 };
	uint32_t signals_real_cycles[0x100 * 8] = { 0 };
	struct pci_device *dev = nva_cards[cnum].pci;
	volatile uint8_t *bar1, val;
	int ret, i, e;

	ret = pci_device_map_range(dev, dev->regions[1].base_addr, dev->regions[1].size, PCI_DEV_MAP_FLAG_WRITABLE, (void**)&bar1);
	if (ret) {
		printf("HOST_MEM: Failed to mmap bar1. aborting.\n");
		return;
	}

	printf("<HOST_MEM (1000 rd, 200 wr @bar1[0])> /!\\ no drivers should be loaded!\n");

	/* let's create some activity  */
	for (i = 0; i < 0xff; i+=4) {
		nv40_start_monitoring(cnum, 0, i, i+1, i+2, i+3);
		for (e = 0; e < 1000; e++)
			val = bar1[0];
		for (e = 0; e < 200; e++)
			bar1[0] = val;
		nv40_stop_monitoring(cnum, 0, NULL, signals_real, signals_real_cycles);
	}

	for (i = 0; i < 0xff; i++) {
		uint32_t val = SIGNAL_VALUE(signals_real, 0, i);
		uint32_t cycles = SIGNAL_VALUE(signals_real_cycles, 0, i);
		if (val == 200)
			print_counter_value("HOST_MEM", "WR", 0, i, val, cycles);
		else if (val == 1000)
			print_counter_value("HOST_MEM", "RD", 0, i, val, cycles);
		else if (val == 1200)
			print_counter_value("HOST_MEM", "TRANS", 0, i, val, cycles);
		else if (val == 3000)
			print_counter_value("HOST_MEM", "3_RD", 0, i, val, cycles);
		else if (val >= 10 && val < 10000)
			print_counter_value("UNK", "", 0, i, val, cycles);
	}

	printf("</HOST_MEM>\n\n");
}

void find_pgraphIdle_and_interrupt(int cnum)
{
	unsigned char signals_idle[0x100 * 8];
	struct signals_comparaison diffs;
	int i;

	uint32_t r_400500, r_400808, r_40013c;

	printf("<PGRAPH_IDLE/INTERRUPT> /!\\ no drivers should be loaded!\n");

	/* safety check:  */
	if (nva_rd32(cnum, 0x140) == 1) {
		printf("You shouldn't run this tool while a driver is running\n");
		goto error;
	}

	/* reboot PGRAPH */
	nva_mask(cnum, 0x200, 0x00201000, 0x00000000);
	nva_mask(cnum, 0x200, 0x00201000, 0x00201000);

	r_400500 =  nva_rd32(cnum, 0x400500);
	r_400808 = nva_rd32(cnum, 0x400808);
	r_40013c = nva_rd32(cnum, 0x40013c);

	/* generate an illegal method IRQ
	 * that will pull the PGRAP_IDLE signal down and the PGRAPH_INTERRUPT up
	 *
	 * neither nouveau or nvidia should be loaded as they would ack the interrupt
	 * straight away.
	 */
	nva_wr32(cnum, 0x400500, 0x10001);
	nva_wr32(cnum, 0x400808, 0xa00000fc);
	nva_wr32(cnum, 0x40013c, 0xffffffff);

	poll_signals(cnum, signals_idle);
	diffs = signals_compare(signals_ref, signals_idle);

	if (diffs.diff_count >= 1) {
		for (i = 0; i < diffs.diff_count; i++) {
			uint8_t set, signal;

			set = diffs.differences[i].set;
			signal = diffs.differences[i].signal;

			if (diffs.differences[i].set == 1) {
				if (diffs.differences[i].change == ONE_TO_ZERO)
					printf("PGRAPH_IDLE: Set %u, signal 0x%.2x\n", set, signal);
				else if (diffs.differences[i].change == ZERO_TO_ONE)
					printf("PGRAPH_INTERRUPT: Set %u, signal 0x%.2x\n", set, signal);
			} else {
				printf("Unexpected difference: ");
				print_difference(diffs.differences[i]);
			}
		}

		signals_comparaison_free(diffs);
	} else
		printf("Not found. Please re-run when the GPU is idle.\n");

	/* restore our mess */
	nva_wr32(cnum, 0x400500, r_400500);
	nva_wr32(cnum, 0x400808, r_400808);
	nva_wr32(cnum, 0x40013c, r_40013c);

	/* ACK all PGRAPH's interrupts */
	nva_wr32(cnum, 0x400100, 0xffffffff);

error:
	printf("</PGRAPH_IDLE/INTERRUPT>\n\n");
}

void find_ctxCtlFlags(int cnum)
{
	uint8_t signals_ref_ctx[0x100 * 8];
	uint8_t signals_tmp[0x100 * 8];
	struct signals_comparaison diffs;
	int bit, i;
	uint32_t r_400824 = nva_rd32(cnum, 0x400824);

	printf("<CTXCTL FLAGS>\n");

	nva_mask(cnum, 0x400824, 0xf0000000, 0);
	poll_signals(cnum, signals_ref_ctx);

	for (bit = 28; bit < 32; bit++) {
		nva_mask(cnum, 0x400824, 0xf0000000, 1 << bit);

		poll_signals(cnum, signals_tmp);
		diffs = signals_compare(signals_ref_ctx, signals_tmp);

		if (diffs.diff_count >= 1) {
			for (i = 0; i < diffs.diff_count; i++) {
				uint8_t set, signal;

				set = diffs.differences[i].set;
				signal = diffs.differences[i].signal;

				if (diffs.differences[i].set == 1) {
					if (diffs.differences[i].change == ZERO_TO_ONE)
						printf("CTXCTL flag 0x%.2x: Set %u, signal 0x%.2x\n", bit, set, signal);
				} else {
					printf("Unexpected difference: ");
					print_difference(diffs.differences[i]);
				}
			}
		} else
			printf("Not found. Please re-run when the GPU is idle.\n");

		signals_comparaison_free(diffs);
	}

	nva_wr32(cnum, 0x400824, r_400824);

	printf("</CTXCTL FLAGS>\n\n");
}

int main(int argc, char **argv)
{
	int c, cnum = 0;

	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt (argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
		}

	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	if (nva_cards[cnum].chipset.chipset < 0x10 ||
	    nva_cards[cnum].chipset.chipset >= 0xc0)
	{
		fprintf(stderr, "The chipset nv%x isn't currently supported\n",
			nva_cards[cnum].chipset.chipset);
		return 1;
	}

	/* Init */
	nva_wr32(cnum, 0x200, 0xffffffff);

	printf("Chipset nv%x:\n\n", nva_cards[cnum].chipset.chipset);

	poll_signals(cnum, signals_ref);
	find_counter_noise(cnum);
	find_ptimer_b12(cnum);
	find_host_mem_read_write(cnum);
	find_mmio_read_write(cnum, 0x200, "MMIO");
	find_mmio_read_write(cnum, 0x2210, "MMIO_PFIFO");
	find_mmio_read_write(cnum, 0x610384, "MMIO_PDISPLAY");
	find_mmio_read_write(cnum, 0x6666, "MMIO_INVALID");
	find_pgraphIdle_and_interrupt(cnum);
	find_ctxCtlFlags(cnum);

	return 0;
}
