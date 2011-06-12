#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "nva.h"
#include "rnn.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif
typedef u64 ptime_t;
#include <time.h>

#define NV04_PTIMER_TIME_0                                 0x00009400
#define NV04_PTIMER_TIME_1                                 0x00009410

ptime_t time_diff_us(struct timeval start, struct timeval end)
{
	return ((end.tv_sec) - (start.tv_sec))*1000000 + ((end.tv_usec) - (start.tv_usec));
}

ptime_t get_time(unsigned int card)
{
	ptime_t low;

	/* From kmmio dumps on nv28 this looks like how the blob does this.
	* It reads the high dword twice, before and after.
	* The only explanation seems to be that the 64-bit timer counter
	* advances between high and low dword reads and may corrupt the
	* result. Not confirmed.
	*/
	ptime_t high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	ptime_t high1;
	do {
		high1 = high2;
		low = nva_rd32(card, NV04_PTIMER_TIME_0);
		high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	} while (high1 != high2);
	return ((((ptime_t)high2) << 32) | (ptime_t)low) >> 5;
}

void time_pcounter(unsigned int cnum)
{
	if (nva_cards[cnum].card_type != 0x50 || nva_cards[cnum].chipset == 0x50) {
		printf("pcounter is unsupported on your card\n");
		return;
	}

	printf ("Perf counters:\n");

	int i;
	for (i = 0; i < 8; i++) {
		nva_wr32(cnum, 0xa7c0 + i * 4, 1);
		nva_wr32(cnum, 0xa420 + i * 4, 0xffff);
	}
		sleep(1);
	for (i = 0; i < 8; i++)
		nva_wr32(cnum, 0xa420 + i * 4, 0xffff);
	for (i = 0; i < 8; i++)
		printf ("Set %d: %d Hz\n", i, nva_rd32(cnum, 0xa600 + i * 4));
}

void time_pgraph_dispatch_clock(unsigned int card)
{
	struct timeval start, end;
	ptime_t t_start, t_end;
	u32 reg;

	if (nva_cards[card].card_type == 0x50)
		reg = 0x4008f8;
	else if (nva_cards[card].card_type == 0xc0)
		reg = 0x4040f4;
	else {
		printf("pgraph_dispatch_clock is only available on nv50+ chipsets\n");
		return;
	}

	gettimeofday(&start, NULL);

	t_start = nva_rd32(card, reg);

	do
	{
		gettimeofday(&end, NULL);
	} while (time_diff_us(start, end) < 1000000);

	gettimeofday(&end, NULL);
	t_end = nva_rd32(card, reg);

	printf("PGRAPH.DISPATCH's clock: 1s = %d cycles --> frequency = %f MHz\n",
	       (t_end - t_start), (t_end - t_start)/1000000.0);
}

ptime_t time_ptimer_clock(unsigned int card)
{
	struct timeval start, end;
	ptime_t t_start, t_end;

	gettimeofday(&start, NULL);
	t_start = get_time(card);

	do
	{
		gettimeofday(&end, NULL);
	} while (time_diff_us(start, end) < 1000000);

	gettimeofday(&end, NULL);
	t_end = get_time(card);

	return t_end - t_start;
}

void time_ptimer(unsigned int card)
{
	ptime_t ptimer_boot, ptimer_default, ptimer_max, ptimer_calibrated;

	/* Save the current values */
	u32 r9200 = nva_rd32(card, 0x9200);
	u32 r9210 = nva_rd32(card, 0x9210);
	u32 r9220 = nva_rd32(card, 0x9220);

	ptimer_boot = time_ptimer_clock(card);
	printf("PTIMER's boot: 1s = %d cycles --> frequency = %f MHz\n",
	       ptimer_boot, ptimer_boot/1000000.0);

	/* Calibrate to standard frequency (27MHz?) */
	nva_wr32(card, 0x9200, 0x1);
	nva_wr32(card, 0x9210, 0x1);
	nva_wr32(card, 0x9220, 0x0);

	ptimer_default = time_ptimer_clock(card);
	printf("PTIMER's clock source: 1s = %d cycles --> frequency = %f MHz\n",
	       ptimer_default, ptimer_default/1000000.0);

	if (nva_cards[card].card_type >= 0x40) {
		/* Calibrate to max frequency */
		nva_wr32(card, 0x9200, 0x1);
		nva_wr32(card, 0x9210, 0x1);
		nva_wr32(card, 0x9220, 0x10000);

		ptimer_max = time_ptimer_clock(card);
		printf("PTIMER's clock source (max): 1s = %d cycles --> frequency = %f MHz\n",
		ptimer_max, ptimer_max/1000000.0);
	}

	/* Calibrate to read in µs */
	nva_wr32(card, 0x9200, 27);
	nva_wr32(card, 0x9210, 0x1);
	nva_wr32(card, 0x9220, 0x0);

	ptimer_calibrated = time_ptimer_clock(card);
	printf("PTIMER calibrated: 1s = %d cycles --> frequency = %f MHz\n",
	       ptimer_calibrated, ptimer_calibrated/1000000.0);

	/* Restore the previous values */
	nva_wr32(card, 0x9200, r9200);
	nva_wr32(card, 0x9210, r9210);
	nva_wr32(card, 0x9220, r9210);
}

void time_fuc_engine_periodic (unsigned int card, const char *fuc_engine_name, u32 fucengine)
{
	struct timeval start, end;
	ptime_t t_start, t_end;

	/* Save the current values */
	u32 r24 = nva_rd32(card, fucengine + 0x24);
	u32 r28 = nva_rd32(card, fucengine + 0x28);

	nva_wr32(card, fucengine + 0x28, 0x1);
	nva_wr32(card, fucengine + 0x24, 0xffffffff);
	do
	{
		t_start = nva_rd32(card, fucengine + 0x24);
	} while (t_start < 10000000);

	gettimeofday(&start, NULL);

	while (t_start - nva_rd32(card, fucengine + 0x24) < 10000000);

	gettimeofday(&end, NULL);

	ptime_t val = time_diff_us(start, end);
	printf("%s's periodic timer: frequency = %f MHz\n", fuc_engine_name, 10000000.0/val);

	/* Restore the previous values */
	nva_wr32(card, fucengine + 0x28, r28);
	nva_wr32(card, fucengine + 0x24, r24);
}

void time_fuc_engine_watchdog (unsigned int card, const char *fuc_engine_name, u32 fucengine)
{
	struct timeval start, end;
	ptime_t t_start, t_end;

	/* Save the current values */
	u32 r34 = nva_rd32(card, fucengine + 0x34);
	u32 r38 = nva_rd32(card, fucengine + 0x38);

	nva_wr32(card, fucengine + 0x38, 0x1);
	nva_wr32(card, fucengine + 0x34, 0xffffffff);
	do
	{
		t_start = nva_rd32(card, fucengine + 0x34);
	} while (t_start < 10000000);

	gettimeofday(&start, NULL);

	while (t_start - nva_rd32(card, fucengine + 0x34) < 10000000);

	gettimeofday(&end, NULL);

	ptime_t val = time_diff_us(start, end);
	printf("%s's watchdog: frequency       = %f MHz\n", fuc_engine_name, 10000000.0/val);

	/* Restore the previous values */
	nva_wr32(card, fucengine + 0x38, r38);
	nva_wr32(card, fucengine + 0x34, r34);
}

int main(int argc, char **argv)
{
	struct nva_card *card = NULL;
	u32 pmc_enable;
	char c, cnum = 0;

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
	} else
		card = &nva_cards[cnum];

	printf("Using card nv%x\n\n", card->chipset);

	time_ptimer(cnum);
	printf("\n");

	time_pgraph_dispatch_clock(cnum);
	printf("\n");

	time_pcounter(cnum);
	printf("\n");

	if (card->chipset < 0x98 || card->chipset == 0xa0) {
		printf("Your card doesn't support fuc (chipset > nv98+ && chipset != nva0 needed)\n");
		return 0;
	}

	/* activate all the engines */
	pmc_enable = nva_rd32(cnum, 0x200);
	nva_wr32(cnum, 0x200, 0xffffffff);

	time_fuc_engine_periodic(cnum, "PBSP", 0x84000);
	time_fuc_engine_watchdog(cnum, "PBSP", 0x84000);
	printf("\n");

	time_fuc_engine_periodic(cnum, "PVP", 0x85000);
	time_fuc_engine_watchdog(cnum, "PVP", 0x85000);
	printf("\n");

	time_fuc_engine_periodic(cnum, "PPPP", 0x86000);
	time_fuc_engine_watchdog(cnum, "PPPP", 0x86000);
	printf("\n");

	if (card->chipset == 0x98 || card->chipset == 0xaa || card->chipset == 0xac) {
		time_fuc_engine_periodic(cnum, "PCRYPT", 0x87000);
		time_fuc_engine_watchdog(cnum, "PCRYPT", 0x87000);
		printf("\n");
	} else {
		printf("Your card doesn't support PCOPY (chipset < nva3 only)\n\n");
	}

	if (card->chipset >= 0xa3 && card->chipset != 0xaa && card->chipset != 0xac) {
		time_fuc_engine_periodic(cnum, "PCOPY[0]", 0x104000);
		time_fuc_engine_watchdog(cnum, "PCOPY[0]", 0x104000);
		printf("\n");
	} else {
		printf("Your card doesn't support PCOPY[0] (nva3+ only)\n\n");
	}

	if (card->chipset >= 0xc0) {
		time_fuc_engine_periodic(cnum, "PCOPY[1]", 0x105000);
		time_fuc_engine_watchdog(cnum, "PCOPY[1]", 0x105000);
		printf("\n");
	} else {
		printf("Your card doesn't support PCOPY[0] (nva3+ only)\n\n");
	}

	if (card->chipset >= 0xa3 && card->chipset != 0xaa && card->chipset != 0xac) {
		time_fuc_engine_periodic(cnum, "PDAEMON", 0x10a000);
		time_fuc_engine_watchdog(cnum, "PDAEMON", 0x10a000);
		printf("\n");
	} else {
		printf("Your card doesn't support PDAEMON (nva3+ only)\n\n");
	}

	if (card->chipset == 0xaf) {
		time_fuc_engine_periodic(cnum, "PUNK1C1", 0x1c1000);
		time_fuc_engine_watchdog(cnum, "PUNK1C1", 0x1c1000);
		printf("\n");
	} else {
		printf("Your card doesn't support PUNK1C1 (nvaf only)\n\n");
	}

	if (card->chipset >= 0xc0) {
		time_fuc_engine_periodic(cnum, "PGRAPH.CTXCTL", 0x409000);
		time_fuc_engine_watchdog(cnum, "PGRAPH.CTXCTL", 0x409000);
		printf("\n");

		time_fuc_engine_periodic(cnum, "PGRAPH.TP[0].CTXCTL", 0x502000);
		time_fuc_engine_watchdog(cnum, "PGRAPH.TP[0].CTXCTL", 0x502000);
		printf("\n");
	} else {
		printf("Your card doesn't support PGRAPH (nvc0+ only)\n\n");
	}

	/* restore PMC enable */
	nva_wr32(cnum, 0x200, pmc_enable);

	return 0;
}
