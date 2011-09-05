#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "nva.h"
#include "rnn.h"

#define SIGNALS_LENGTH 0x100
uint8_t signals_ref[SIGNALS_LENGTH * 8]; // reserve an octet for each bit of the a800 range

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

void print_poll_data(const char *name, uint8_t *signals)
{
	int i, s;

	printf("Print data poll '%s':\n", name);
	for (s = 0; s < 8; s++) {
		printf("Set %i:", s);
		for (i = 0; i < 0x20 * 8; i++) {
			if (i % 0x10 == 0)
				printf("\n0x%02x: ", i % (0x20 * 8));

			printf("%.3i ", signals[s * (0x20 * 8) + i]);

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

	printf("<PTIMER_B17>\n");

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
				printf("Unexepected difference: ");
				print_difference(diffs.differences[i]);
			}
		}
	} else
		printf("Not found. Please re-run when the GPU is idle.\n");

	signals_comparaison_free(diffs);

	printf("</PTIMER_B17>\n\n");
}

void find_pgraphIdle_and_interrupt(int cnum)
{
	unsigned char signals_idle[0x100 * 8];
	struct signals_comparaison diffs;
	int i;

	uint32_t r_400500, r_400808, r_40013c;

	r_400500 =  nva_rd32(cnum, 0x400500);
	r_400808 = nva_rd32(cnum, 0x400808);
	r_40013c = nva_rd32(cnum, 0x40013c);

	nva_wr32(cnum, 0x400500, 0x10001);
	nva_wr32(cnum, 0x400808, 0xa000fffc);
	nva_wr32(cnum, 0x40013c, 0xffffffff);

	printf("<PGRAPH_IDLE/INTERRUPT>\n");

	poll_signals(cnum, signals_idle);
	diffs = signals_compare(signals_ref, signals_idle);

	if (diffs.diff_count > 2) {
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
				printf("Unexepected difference: ");
				print_difference(diffs.differences[i]);
			}
		}

		signals_comparaison_free(diffs);
	} else
		printf("Not found. Please re-run when the GPU is idle.\n");

	nva_wr32(cnum, 0x400500, r_400500);
	nva_wr32(cnum, 0x400808, r_400808);
	nva_wr32(cnum, 0x40013c, r_40013c);

	/* ACK all the PGRAPH interrupts */
	nva_wr32(cnum, 0x400100, 0xffffffff);

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
					printf("Unexepected difference: ");
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

	if (nva_cards[cnum].chipset < 0x10 ||
	    nva_cards[cnum].chipset >= 0xc0)
	{
		fprintf(stderr, "The chipset nv%x isn't currently supported\n",
			nva_cards[cnum].chipset);
		return 1;
	}

	/* Init */
	nva_wr32(cnum, 0x200, 0xffffffff);

	printf("Chipset nv%x:\n\n", nva_cards[cnum].chipset);

	poll_signals(cnum, signals_ref);
	find_ptimer_b12(cnum);
	find_pgraphIdle_and_interrupt(cnum);
	find_ctxCtlFlags(cnum);

	return 0;
}
